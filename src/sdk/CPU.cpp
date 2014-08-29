/**
 * @file CPU.cpp
 * @author Minmin Gong
 *
 * @section DESCRIPTION
 *
 * This source file is part of CPUTSDK, a subproject of CPU-T
 * For the latest info, see http://www.klayge.org
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * You may alternatively use this source under the terms of
 * the KlayGE Proprietary License (KPL). You can obtained such a license
 * from http://www.klayge.org/licensing/.
 */

#include <CPU-T/CPU.hpp>

#include <windows.h>
#if (_WIN32_WINNT >= 0x0603 /*_WIN32_WINNT_WINBLUE*/)
#include <VersionHelpers.h>
#endif
#include <cstring>
#include <cstdio>
#include <cassert>
#include <vector>
#include <algorithm>

#include <cstdint>
namespace CPUT
{
	using std::uint64_t;
	using std::uint32_t;
	using std::uint16_t;
	using std::uint8_t;
	using std::int64_t;
	using std::int32_t;
	using std::int16_t;
	using std::int8_t;
}

#include <intrin.h>

#ifdef CPUT_CPU_X86
#define RDMSR __asm _emit 0x0F __asm _emit 0x32
#endif

namespace
{
#include "CPUIdentifier.cpp"
#include "CacheIdentifier.cpp"

	int Cy6x86PR[6][2] =
	{
		{80, 90},
		{100, 120},
		{110, 133},
		{120, 150},
		{133, 166},
		{150, 200}
	};
	int Cy6x86MXPR[16][2] =
	{
		{100, 133},
		{110, 133},
		{120, 150},
		{125, 150},
		{133, 166},
		{138, 166},
		{150, 166},
		{150, 166},
		{150, 200},
		{165, 200},
		{166, 200},
		{180, 200},
		{188, 233},
		{200, 233},
		{166, 233},
		{208, 266}
	};
	int AMDPR[30][2] =
	{
		{ 72, 75, },
		{ 78, 75, },
		{ 87, 90, },
		{ 93, 90, },
		{ 97, 100, },
		{ 103, 100, },
		{ 113, 120, },
		{ 119, 120, },
		{ 130, 133, },
		{ 136, 133, },
		{ 163, 166, },
		{ 169, 166, },
		{ 177, 180, },
		{ 183, 180, },
		{ 197, 200, },
		{ 203, 200, },
		{ 231, 233, },
		{ 236, 233, },
		{ 245, 250, },
		{ 263, 266, },
		{ 269, 266, },
		{ 287, 300, },
		{ 315, 300, },
		{ 316, 333, },
		{ 330, 333, },
		{ 350, 350, },
		{ 363, 366, },
		{ 377, 380, },
		{ 400, 400, },
		{ 450, 450, }
	};


#ifndef CPUT_CPU_ARM
	void get_cpuid(uint32_t* peax, uint32_t* pebx, uint32_t* pecx, uint32_t* pedx)
	{	
#ifdef CPUT_COMPILER_MSVC
	#if CPUT_COMPILER_VERSION >= 90 
		int CPUInfo[4];
		__cpuidex(CPUInfo, *peax, *pecx);
		*peax = CPUInfo[0];
		*pebx = CPUInfo[1];
		*pecx = CPUInfo[2];
		*pedx = CPUInfo[3];
	#else
		__asm
		{
			mov		eax, [peax]
			mov		ecx, [pecx]
			cpuid
			mov		[peax], eax
			mov		[pebx], ebx
			mov		[pecx], ecx
			mov		[pedx], edx
		}
	#endif
#elif defined CPUT_COMPILER_GCC
	#ifdef CPUT_CPU_X64
		__asm__
		(
			"cpuid"
			: "=a" (*peax), "=b" (*pebx), "=c" (*pecx), "=d" (*pedx)
			: "a" (*peax), "c" (*pecx)
		);
	#else
		__asm__
		(
			"pushl  %%ebx			\n\t"
			"cpuid					\n\t"
			"movl   %%ebx, %%edi	\n\t"
			"popl   %%ebx			\n\t"
			: "=a" (*peax), "=D" (*pebx), "=c" (*pecx), "=d" (*pedx)
			: "a" (*peax), "c" (*pecx)
		);
	#endif
#else
		// TODO: Supports other compiler
#endif
	}
#endif
	

#if (defined(CPUT_CPU_X86) || defined(CPUT_CPU_X64)) && !defined(CPUT_PLATFORM_ANDROID)
	enum CPUIDFeatureMask
	{
		// In EBX of type 1. Intel only.
		CFM_LogicalProcessorCount_Intel = 0x00FF0000,
		CFM_ApicId_Intel = 0xFF000000,

		// In ECX of type 1
		CFM_SSE3		= 1UL << 0,		// SSE3
		CFM_SSSE3		= 1UL << 9,		// SSSE3
		CFM_FMA3		= 1UL << 12,	// 256-bit FMA (Intel Haswell, AMD Piledriver)
		CFM_CMPXCHG16B	= 1UL << 13,	// CMPXCHG16B instruction
		CFM_SSE41		= 1UL << 19,	// SSE4.1 (Intel Core 2 Penryn, Intel Core i7 Nehalem, AMD Bulldozer)
		CFM_SSE42		= 1UL << 20,	// SSE4.2 (Intel Core i7 Nehalem, AMD Bulldozer)
		CFM_MOVBE		= 1UL << 22,	// MOVBE (Intel)
		CFM_POPCNT		= 1UL << 23,	// POPCNT
		CFM_AES			= 1UL << 25,	// AES support (Intel)
		CFM_OSXSAVE		= 1UL << 27,	// OSX save
		CFM_AVX			= 1UL << 28,	// 256-bit AVX (Intel Sandy Bridge, AMD Bulldozer)
		CFM_F16C		= 1UL << 29,	// F16C (Intel Ivy Bridge, AMD Piledriver)

		// In EDX of type 1
		CFM_CMPXCHG8B	= 1UL << 8,		// CMPXCHG8B instruction
		CFM_SYSENTER	= 1UL << 11,	// SYSENTER/SYSEXIT instructions
		CFM_CMOV		= 1UL << 15,	// Conditional move instruction
		CFM_MMX			= 1UL << 23,	// MMX technology
		CFM_SSE			= 1UL << 25,	// SSE
		CFM_SSE2		= 1UL << 26,	// SSE2
		CFM_HTT			= 1UL << 28,	// Hyper-threading technology
		CFM_3DNowExt_AMD	= 1UL << 30,	// 3DNow! extension (AMD only)
		CFM_3DNow_AMD		= 1UL << 31,	// 3DNow! (AMD only)


		// In EBX of type 7
		CFM_AVX2		= 1UL << 5,

		// In EAX of type 4. Intel only.
		CFM_NC_Intel                = 0xFC000000,

		// In ECX of type 0x80000001. AMD only.
		CFM_CmpLegacy_AMD           = 0x00000002,
		CFM_LZCNT_AMD				= 1UL << 5,
		CFM_SSE4A_AMD				= 1UL << 6,
		CFM_MisalignedSSE_AMD		= 1UL << 7,
		CFM_FMA4_AMD				= 1UL << 16,	// FMA4 (AMD Bulldozer)

		// In EDX of type 0x80000001
		CFM_X64						= 1UL << 29,

		// In ECX of type 0x80000008. AMD only.
		CFM_NC_AMD                  = 0x000000FF,
		CFM_ApicIdCoreIdSize_AMD    = 0x0000F000,
	};

#ifdef CPUT_PLATFORM_WINDOWS
#ifdef CPUT_COMPILER_GCC
	typedef enum _LOGICAL_PROCESSOR_RELATIONSHIP
	{
		RelationProcessorCore,
		RelationNumaNode,
		RelationCache
	} LOGICAL_PROCESSOR_RELATIONSHIP;

	typedef enum _PROCESSOR_CACHE_TYPE
	{
		CacheUnified,
		CacheInstruction,
		CacheData,
		CacheTrace
	} PROCESSOR_CACHE_TYPE;

	typedef struct _CACHE_DESCRIPTOR
	{
		BYTE   Level;
		BYTE   Associativity;
		WORD   LineSize;
		DWORD  Size;
		PROCESSOR_CACHE_TYPE Type;
	} CACHE_DESCRIPTOR, *PCACHE_DESCRIPTOR;

	typedef struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION
	{
		ULONG_PTR   ProcessorMask;
		LOGICAL_PROCESSOR_RELATIONSHIP Relationship;
		union
		{
			struct
			{
				BYTE  Flags;
			} ProcessorCore;
			struct
			{
				DWORD NodeNumber;
			} NumaNode;
			CACHE_DESCRIPTOR Cache;
			ULONGLONG  Reserved[2];
		};
	} SYSTEM_LOGICAL_PROCESSOR_INFORMATION, *PSYSTEM_LOGICAL_PROCESSOR_INFORMATION;
#endif

	typedef BOOL (WINAPI* GetLogicalProcessorInformationPtr)(SYSTEM_LOGICAL_PROCESSOR_INFORMATION*, uint32_t*);
#endif

	class ApicExtractor
	{
	public:
		ApicExtractor(uint8_t log_procs_per_pkg = 1, uint8_t cores_per_pkg = 1)
		{
			this->SetPackageTopology(log_procs_per_pkg, cores_per_pkg);
		}

		uint8_t SmtId(uint8_t apic_id) const
		{
			return apic_id & smt_id_mask_.mask;
		}

		uint8_t CoreId(uint8_t apic_id) const
		{
			return (apic_id & core_id_mask_.mask) >> smt_id_mask_.width;
		}

		uint8_t PackageId(uint8_t apic_id) const
		{
			return (apic_id & pkg_id_mask_.mask) >> (smt_id_mask_.width + core_id_mask_.width);
		}

		uint8_t PackageCoreId(uint8_t apic_id) const
		{
			return (apic_id & (pkg_id_mask_.mask | core_id_mask_.mask)) >> smt_id_mask_.width;
		}

		uint8_t LogProcsPerPkg() const
		{
			return log_procs_per_pkg_;
		}

		uint8_t CoresPerPkg() const
		{
			return cores_per_pkg_;
		}

		void SetPackageTopology(uint8_t log_procs_per_pkg, uint8_t cores_per_pkg)
		{
			log_procs_per_pkg_	= log_procs_per_pkg;
			cores_per_pkg_		= cores_per_pkg;

			smt_id_mask_.width	= this->GetMaskWidth(log_procs_per_pkg_ / cores_per_pkg_);
			core_id_mask_.width	= this->GetMaskWidth(cores_per_pkg_);
			pkg_id_mask_.width	= 8 - (smt_id_mask_.width + core_id_mask_.width);

			pkg_id_mask_.mask	= static_cast<uint8_t>(0xFF << (smt_id_mask_.width + core_id_mask_.width));
			core_id_mask_.mask	= static_cast<uint8_t>((0xFF << smt_id_mask_.width) ^ pkg_id_mask_.mask);
			smt_id_mask_.mask	= static_cast<uint8_t>(~(0xFF << smt_id_mask_.width));
		}

	private:
		static uint8_t GetMaskWidth(uint8_t max_ids)
		{
			-- max_ids;

			// find index of msb
			uint8_t msb_idx = 8;
			uint8_t msb_mask = 0x80;
			while (msb_mask && !(msb_mask & max_ids))
			{
				-- msb_idx;
				msb_mask >>= 1;
			}
			return msb_idx;
		}

	private:
		struct id_mask_t
		{
			uint8_t width;
			uint8_t mask;
		};

		uint8_t		log_procs_per_pkg_;
		uint8_t		cores_per_pkg_;
		id_mask_t	smt_id_mask_;
		id_mask_t	core_id_mask_;
		id_mask_t	pkg_id_mask_;
	};

	class Cpuid
	{
	public:
		Cpuid()
			: eax_(0), ebx_(0), ecx_(0), edx_(0)
		{
		}

		uint32_t Eax() const
		{
			return eax_;
		}
		uint32_t Ebx() const
		{
			return ebx_;
		}
		uint32_t Ecx() const
		{
			return ecx_;
		}
		uint32_t Edx() const
		{
			return edx_;
		}

		void Call(uint32_t fn)
		{
			eax_ = fn;
			get_cpuid(&eax_, &ebx_, &ecx_, &edx_);
		}

	private:
		uint32_t eax_;
		uint32_t ebx_;
		uint32_t ecx_;
		uint32_t edx_;
	};

	char const GenuineIntel[] = "GenuineIntel";
	char const AuthenticAMD[] = "AuthenticAMD";
#endif
}

namespace CPUT
{
	CPUInfo::CPUInfo()
		: feature_mask_(0)
	{
		memset(vendor_, 0, sizeof(vendor_));
		memset(brand_string_, 0, sizeof(brand_string_));

		num_hw_threads_ = 1;
		num_cores_ = 1;

#if (defined(CPUT_CPU_X86) || defined(CPUT_CPU_X64)) && !defined(CPUT_PLATFORM_ANDROID)
		this->DumpCPUIDs();

		*reinterpret_cast<uint32_t*>(&vendor_[0]) = this->CPUIDResult(0, 1);
		*reinterpret_cast<uint32_t*>(&vendor_[4]) = this->CPUIDResult(0, 3);
		*reinterpret_cast<uint32_t*>(&vendor_[8]) = this->CPUIDResult(0, 2);

		if (this->MaxStdFn() >= 1)
		{
			uint32_t id = this->CPUIDResult(1, 0);
			type_ = (id & 0x00003000) >> 12;
			family_ = (id & 0x00000F00) >> 8;
			model_ = (id & 0x000000F0) >> 4;
			uint32_t ext_family = (id & 0x0FF00000) >> 20;
			uint32_t ext_model = (id & 0x000F0000) >> 16;
			if (0x0F == family_)
			{
				family_ += ext_family;
			}
			if ((0x06 == family_) || (0x0F == family_))
			{
				model_ |= (ext_model << 4);
			}
			stepping_ = id & 0x0000000F;

			if (this->CPUIDResult(1, 0) & 0x00040000)
			{
				sprintf(serial_number_, "%d%d%d", id, this->CPUIDResult(3, 3), this->CPUIDResult(3, 2));
			}

			feature_mask_ |= this->CPUIDResult(1, 3) & CFM_CMPXCHG8B ? CF_CMPXCHG8B : 0;
			feature_mask_ |= this->CPUIDResult(1, 3) & CFM_SYSENTER ? CF_SYSENTER : 0;
			feature_mask_ |= this->CPUIDResult(1, 3) & CFM_CMOV ? CF_CMOV : 0;
			feature_mask_ |= this->CPUIDResult(1, 3) & CFM_MMX ? CF_MMX : 0;
			feature_mask_ |= this->CPUIDResult(1, 3) & CFM_SSE ? CF_SSE : 0;
			feature_mask_ |= this->CPUIDResult(1, 3) & CFM_SSE2 ? CF_SSE2 : 0;
			feature_mask_ |= this->CPUIDResult(1, 2) & CFM_SSE3 ? CF_SSE3 : 0;
			feature_mask_ |= this->CPUIDResult(1, 3) & CFM_HTT ? CF_HTT : 0;
			feature_mask_ |= this->CPUIDResult(1, 2) & CFM_SSSE3 ? CF_SSSE3 : 0;
			feature_mask_ |= this->CPUIDResult(1, 2) & CFM_SSE41 ? CF_SSE41 : 0;
			feature_mask_ |= this->CPUIDResult(1, 2) & CFM_SSE42 ? CF_SSE42 : 0;
			feature_mask_ |= (this->CPUIDResult(1, 2) & CFM_OSXSAVE) && (this->CPUIDResult(1, 2) & CFM_FMA3 ? CF_FMA3 : 0);
			feature_mask_ |= this->CPUIDResult(1, 2) & CFM_CMPXCHG16B ? CF_CMPXCHG16B : 0;
			feature_mask_ |= this->CPUIDResult(1, 2) & CFM_MOVBE ? CF_MOVBE : 0;
			feature_mask_ |= this->CPUIDResult(1, 2) & CFM_POPCNT ? CF_POPCNT : 0;
			feature_mask_ |= this->CPUIDResult(1, 2) & CFM_AES ? CF_AES : 0;
			feature_mask_ |= (this->CPUIDResult(1, 2) & CFM_OSXSAVE) && (this->CPUIDResult(1, 2) & CFM_AVX) ? CF_AVX : 0;
			feature_mask_ |= (this->CPUIDResult(1, 2) & CFM_OSXSAVE) && (this->CPUIDResult(1, 2) & CFM_F16C ? CF_F16C : 0);
			if (0 == strcmp(AuthenticAMD, vendor_))
			{
				feature_mask_ |= this->CPUIDResult(1, 3) & CFM_3DNow_AMD ? CF_3DNow : 0;
				feature_mask_ |= this->CPUIDResult(1, 3) & CFM_3DNowExt_AMD ? CF_3DNowExt : 0;
			}

			if (this->MaxStdFn() >= 7)
			{
				feature_mask_ |= this->CPUIDResult(7, 1) & CFM_AVX2 ? CF_AVX2 : 0;
			}
		}

		if (this->MaxExtFn() >= 0x80000001)
		{
			if (0 == strcmp(AuthenticAMD, vendor_))
			{
				feature_mask_ |= this->CPUIDResult(0x80000001, 2) & CFM_LZCNT_AMD ? CF_LZCNT : 0;
				feature_mask_ |= this->CPUIDResult(0x80000001, 2) & CFM_SSE4A_AMD ? CF_SSE4A : 0;
				feature_mask_ |= this->CPUIDResult(0x80000001, 2) & CFM_MisalignedSSE_AMD ? CF_MisalignedSSE : 0;
			}
			feature_mask_ |= this->CPUIDResult(0x80000001, 3) & CFM_X64 ? CF_X64 : 0;
			feature_mask_ |= (this->CPUIDResult(0x80000001, 2) & CFM_OSXSAVE) && (this->CPUIDResult(0x80000001, 2) & CFM_FMA4_AMD ? CF_FMA4 : 0);
		}

		if (this->MaxExtFn() >= 0x80000004)
		{
			*reinterpret_cast<uint32_t*>(&brand_string_[0]) = this->CPUIDResult(0x80000002, 0);
			*reinterpret_cast<uint32_t*>(&brand_string_[4]) = this->CPUIDResult(0x80000002, 1);
			*reinterpret_cast<uint32_t*>(&brand_string_[8]) = this->CPUIDResult(0x80000002, 2);
			*reinterpret_cast<uint32_t*>(&brand_string_[12]) = this->CPUIDResult(0x80000002, 3);

			*reinterpret_cast<uint32_t*>(&brand_string_[16]) = this->CPUIDResult(0x80000003, 0);
			*reinterpret_cast<uint32_t*>(&brand_string_[20]) = this->CPUIDResult(0x80000003, 1);
			*reinterpret_cast<uint32_t*>(&brand_string_[24]) = this->CPUIDResult(0x80000003, 2);
			*reinterpret_cast<uint32_t*>(&brand_string_[28]) = this->CPUIDResult(0x80000003, 3);

			*reinterpret_cast<uint32_t*>(&brand_string_[32]) = this->CPUIDResult(0x80000004, 0);
			*reinterpret_cast<uint32_t*>(&brand_string_[36]) = this->CPUIDResult(0x80000004, 1);
			*reinterpret_cast<uint32_t*>(&brand_string_[40]) = this->CPUIDResult(0x80000004, 2);
			*reinterpret_cast<uint32_t*>(&brand_string_[44]) = this->CPUIDResult(0x80000004, 3);
		}

		for (uint32_t i = 0; i < 4; ++ i)
		{
			uint32_t d = this->CPUIDResult(2, i);
			uint32_t start = (0 == i) ? 8 : 0;
			for (uint32_t offset = start; offset < 32; offset += 8)
			{
				uint32_t code = (d >> offset) & 0xFF;
				if (0xFF == code)
				{
					uint32_t eax, ebx, ecx, edx;
					for (int l = 0; l < 8; ++ l)
					{
						eax = 4;
						ecx = l;
						get_cpuid(&eax, &ebx, &ecx, &edx);

						uint32_t cache_level = (eax >> 5) & 0x7;
						uint32_t cache_type = eax & 0x1F;
						uint32_t way = ((ebx >> 22) & 0x03FF) + 1;
						uint32_t partition = ((ebx >> 12) & 0x03FF) + 1;
						uint32_t line = (ebx & 0x0FFF) + 1;
						uint32_t sets = ecx + 1;
						uint32_t size = (way * partition * line * sets) / 1024;

						switch (cache_level)
						{
						case 1:
							switch (cache_type)
							{
							case 1:
								l1_data_cache_.size = size;
								l1_data_cache_.way = way;
								l1_data_cache_.line = line;
								break;

							case 2:
								l1_code_cache_.size = size;
								l1_code_cache_.way = way;
								l1_code_cache_.line = line;
								break;

							default:
								break;
							}
							break;

						case 2:
							l2_cache_.size = size;
							l2_cache_.way = way;
							l2_cache_.line = line;
							break;

						case 3:
							l3_cache_.size = size;
							l3_cache_.way = way;
							l3_cache_.line = line;
							break;

						default:
							break;
						}
					}
				}
				else
				{
					std::string type;
					std::string page;
					int size;
					int way;
					std::string entry;
					int line;
					CacheIdentify(code, family_, type, page, size, way, entry, line);
					
					if ("L0 data TLB" == type)
					{
						l0_data_tlb_.page = page;
						l0_data_tlb_.way = way;
						l0_data_tlb_.entry = entry;
					}
					else if ("L1 data TLB" == type)
					{
						l1_data_tlb_.page = page;
						l1_data_tlb_.way = way;
						l1_data_tlb_.entry = entry;
					}
					else if ("L2 data TLB" == type)
					{
						l2_data_tlb_.page = page;
						l2_data_tlb_.way = way;
						l2_data_tlb_.entry = entry;
					}
					else if ("Code TLB" == type)
					{
						code_tlb_.page = page;
						code_tlb_.way = way;
						code_tlb_.entry = entry;
					}
					else if ("Data TLB" == type)
					{
						data_tlb_.page = page;
						data_tlb_.way = way;
						data_tlb_.entry = entry;
					}
					else if ("L1 code cache" == type)
					{
						l1_code_cache_.size = size;
						l1_code_cache_.way = way;
						l1_code_cache_.line = line;
					}
					else if ("L1 data cache" == type)
					{
						l1_data_cache_.size = size;
						l1_data_cache_.way = way;
						l1_data_cache_.line = line;
					}
					else if ("L2 cache" == type)
					{
						l2_cache_.size = size;
						l2_cache_.way = way;
						l2_cache_.line = line;
					}
					else if ("L3 cache" == type)
					{
						l3_cache_.size = size;
						l3_cache_.way = way;
						l3_cache_.line = line;
					}
				}
			}
		}

		tech_ = "Unknown";
		transistors_ = "Unknown";
		codename_ = "Unknown";
		package_ = "Unknown";
		CPUIdentify(vendor_, family_, model_, stepping_, cpu_name_, tech_, transistors_, codename_, package_);

		this->UpdateFrequency();

#if defined CPUT_PLATFORM_WINDOWS
		{
			SYSTEM_INFO si;
#if defined CPUT_PLATFORM_WINDOWS_DESKTOP
			::GetSystemInfo(&si);
#else
			::GetNativeSystemInfo(&si);
#endif
			num_hw_threads_ = si.dwNumberOfProcessors;
		}
#elif defined CPUT_PLATFORM_LINUX
		// Linux doesn't easily allow us to look at the Affinity Bitmask directly,
		// but it does provide an API to test affinity maskbits of the current process
		// against each logical processor visible under OS.
		num_hw_threads_ = sysconf(_SC_NPROCESSORS_CONF);	// This will tell us how many CPUs are currently enabled.
#endif

#if defined CPUT_PLATFORM_WINDOWS
#if defined CPUT_PLATFORM_WINDOWS_DESKTOP
		GetLogicalProcessorInformationPtr glpi = nullptr;
		{
#if (_WIN32_WINNT >= 0x0603 /*_WIN32_WINNT_WINBLUE*/)
			if (IsWindowsVistaOrGreater())
#else
			OSVERSIONINFO os_ver_info;
			memset(&os_ver_info, 0, sizeof(os_ver_info));
			os_ver_info.dwOSVersionInfoSize = sizeof(os_ver_info);
			::GetVersionEx(&os_ver_info);

			// There is a bug with the implementation of GetLogicalProcessorInformation
			// on Windows Server 2003 and XP64. Therefore, only
			// GetLogicalProcessorInformation on Windows Vista and up are supported for now.
			if (os_ver_info.dwMajorVersion >= 6)
#endif
			{
				HMODULE hMod = ::GetModuleHandle(TEXT("kernel32"));
				if (hMod)
				{
					glpi = (GetLogicalProcessorInformationPtr)::GetProcAddress(hMod,
						"GetLogicalProcessorInformation");
				}
			}
		}

		if (glpi != nullptr)
		{
			std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> slpi_;

			uint32_t cbBuffer = 0;
			glpi(nullptr, &cbBuffer);

			slpi_.resize(cbBuffer / sizeof(slpi_[0]));
			glpi(&slpi_[0], &cbBuffer);

			num_cores_ = 0;
			for (size_t i = 0; i < slpi_.size(); ++ i)
			{
				if (::RelationProcessorCore == slpi_[i].Relationship)
				{
					++ num_cores_;
				}
			}
		}
		else
		{
			// Indicates if a CpuidImpl object is supported on this platform.
			// Support is only granted on Intel and AMD platforms where the current
			// calling process has security rights to query process affinity and
			// change it if the process and system affinity differ.  CpuidImpl is
			// also not supported if thread affinity cannot be set on systems with
			// more than 1 logical processor.

			bool supported = (0 == strcmp(GenuineIntel, vendor_)) || (0 == strcmp(AuthenticAMD, vendor_));

			if (supported)
			{
				DWORD_PTR process_affinity, system_affinity;
				HANDLE process_handle = ::GetCurrentProcess();

				// Query process affinity mask
				supported = (::GetProcessAffinityMask(process_handle, &process_affinity, &system_affinity) != 0);
				if (supported)
				{
					if (process_affinity != system_affinity)
					{
						// The process and system affinities differ.  Attempt to set
						// the process affinity to the system affinity.
						supported = (::SetProcessAffinityMask(process_handle, system_affinity) != 0);
						if (supported)
						{
							// Restore previous process affinity
							supported = (::SetProcessAffinityMask(process_handle, process_affinity) != 0);
						}
					}

					if (supported && (system_affinity > 1))
					{
						// Attempt to set the thread affinity
						HANDLE thread_handle = ::GetCurrentThread();
						DWORD_PTR thread_affinity = ::SetThreadAffinityMask(thread_handle, process_affinity);
						if (thread_affinity)
						{
							// Restore the previous thread affinity
							supported = (::SetThreadAffinityMask(thread_handle, thread_affinity) != 0);
						}
						else
						{
							supported = false;
						}
					}
				}
			}
#else
		{
			bool supported = false;
#endif
#elif defined CPUT_PLATFORM_LINUX
		{
			bool supported = (0 == strcmp(GenuineIntel, vendor_)) || (0 == strcmp(AuthenticAMD, vendor_));
#endif

			if (supported)
			{
				uint8_t log_procs_per_pkg = 1;
				uint8_t cores_per_pkg = 1;

				// Determine if hyper-threading is enabled.
				if (this->IsFeatureSupport(CF_HTT))
				{
					// Determine the total number of logical processors per package.
					log_procs_per_pkg = static_cast<uint8_t>((this->CPUIDResult(1, 1) & CFM_LogicalProcessorCount_Intel) >> 16);

					// Determine the total number of cores per package.  This info
					// is extracted differently dependending on the cpu vendor.
					if (0 == strcmp(GenuineIntel, vendor_))
					{
						if (this->MaxStdFn() >= 4)
						{
							cores_per_pkg = static_cast<uint8_t>(((this->CPUIDResult(4, 0) & CFM_NC_Intel) >> 26) + 1);
						}
					}
					else
					{
						assert(strcmp(AuthenticAMD, vendor_));

						if (this->MaxExtFn() >= 0x80000008)
						{
							// AMD reports the msb width of the CORE_ID bit field of the APIC ID
							// in ApicIdCoreIdSize_Amd.  The maximum value represented by the msb
							// width is the theoretical number of cores the processor can support
							// and not the actual number of current cores, which is how the msb width
							// of the CORE_ID bit field has been traditionally determined.  If the
							// ApicIdCoreIdSize_Amd value is zero, then you use the traditional method
							// to determine the CORE_ID msb width.
							uint32_t msb_width = this->CPUIDResult(0x80000008, 2) & CFM_ApicIdCoreIdSize_AMD;
							if (msb_width)
							{
								// Set cores_per_pkg to the maximum theortical number of cores
								// the processor package can support (2 ^ width) so the APIC
								// extractor object can be configured to extract the proper
								// values from an APIC.
								cores_per_pkg = static_cast<uint8_t>(1 << ((msb_width >> 12) - 1));
							}
							else
							{
								// Set cores_per_pkg to the actual number of cores being reported
								// by the CPUID instruction.
								cores_per_pkg = static_cast<uint8_t>((this->CPUIDResult(0x80000008, 2) & CFM_NC_AMD) + 1);
							}
						}
					}
				}

				std::vector<uint8_t> apic_ids;

				// Configure the APIC extractor object with the information it needs to
				// be able to decode the APIC.
				ApicExtractor apic_extractor;
				apic_extractor.SetPackageTopology(log_procs_per_pkg, cores_per_pkg);

#if defined CPUT_PLATFORM_WINDOWS
#if defined CPUT_PLATFORM_WINDOWS_DESKTOP
				DWORD_PTR process_affinity, system_affinity;
				HANDLE process_handle = ::GetCurrentProcess();
				HANDLE thread_handle = ::GetCurrentThread();
				::GetProcessAffinityMask(process_handle, &process_affinity, &system_affinity);
				if (1 == system_affinity)
				{
					// Since we only have 1 logical processor present on the system, we
					// can explicitly set a single APIC ID to zero.
					assert(1 == log_procs_per_pkg);
					apic_ids.push_back(0);
				}
				else
				{
					// Set the process affinity to the system affinity if they are not
					// equal so that all logical processors can be accounted for.
					if (process_affinity != system_affinity)
					{
						::SetProcessAffinityMask(process_handle, system_affinity);
					}

					// Call cpuid on each active logical processor in the system affinity.
					DWORD_PTR prev_thread_affinity = 0;
					for (DWORD_PTR thread_affinity = 1; thread_affinity && (thread_affinity <= system_affinity);
						thread_affinity <<= 1)
					{
						if (system_affinity & thread_affinity)
						{
							if (0 == prev_thread_affinity)
							{
								// Save the previous thread affinity so we can return
								// the executing thread affinity back to this state.
								assert(apic_ids.empty());
								prev_thread_affinity = ::SetThreadAffinityMask(thread_handle, thread_affinity);
							}
							else
							{
								assert(!apic_ids.empty());
								::SetThreadAffinityMask(thread_handle, thread_affinity);
							}

							// Allow the thread to switch to masked logical processor.
							::Sleep(0);

							// Store the APIC ID
							apic_ids.push_back(static_cast<uint8_t>((this->CPUIDResult(1, 1) & CFM_ApicId_Intel) >> 24));
						}
					}

					// Restore the previous process and thread affinity state.
					::SetProcessAffinityMask(process_handle, process_affinity);
					::SetThreadAffinityMask(thread_handle, prev_thread_affinity);
					::Sleep(0);
				}
#endif
#elif defined CPUT_PLATFORM_LINUX
				if (1 == num_hw_threads_)
				{
					// Since we only have 1 logical processor present on the system, we
					// can explicitly set a single APIC ID to zero.
					BOOST_ASSERT(1 == log_procs_per_pkg);
					apic_ids.push_back(0);
				}
				else
				{
					cpu_set_t backup_cpu;
					sched_getaffinity(0, sizeof(backup_cpu), &backup_cpu);

					cpu_set_t current_cpu;
					// Call cpuid on each active logical processor in the system affinity.
					for (int j = 0; j < num_hw_threads_; ++ j)
					{
						CPU_ZERO(&current_cpu);
						CPU_SET(j, &current_cpu);
						if (0 == sched_setaffinity(0, sizeof(current_cpu), &current_cpu))
						{
							// Allow the thread to switch to masked logical processor.
							sleep(0);

							// Store the APIC ID
							apic_ids.push_back(static_cast<uint8_t>((this->CPUIDResult(1, 1) & CFM_ApicId_Intel) >> 24));
						}
					}

					// Restore the previous process and thread affinity state.
					sched_setaffinity(0, sizeof(backup_cpu), &backup_cpu);
					sleep(0);
				}
#endif

#if defined CPUT_PLATFORM_WINDOWS_METRO
				num_cores_ = num_hw_threads_;
#else
				std::vector<uint8_t> pkg_core_ids(apic_ids.size());
				for (size_t i = 0; i < apic_ids.size(); ++ i)
				{
					pkg_core_ids[i] = apic_extractor.PackageCoreId(apic_ids[i]);
				}
				std::sort(pkg_core_ids.begin(), pkg_core_ids.end());
				pkg_core_ids.erase(std::unique(pkg_core_ids.begin(), pkg_core_ids.end()), pkg_core_ids.end());
				num_cores_ = static_cast<int>(pkg_core_ids.size());
#endif
			}
		}
#endif
	}

	void CPUInfo::UpdateFrequency()
	{
		LARGE_INTEGER start_time;
		QueryPerformanceCounter(&start_time);

		unsigned __int64 start_cycle = __rdtsc();
		Sleep(750);
		unsigned __int64 cycles = __rdtsc() - start_cycle;

		LARGE_INTEGER end_time;
		QueryPerformanceCounter(&end_time);

		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);

		double duration = static_cast<double>(end_time.QuadPart - start_time.QuadPart) / freq.QuadPart;
		frequency_ = static_cast<int>(cycles / duration / 1000000);
	}

	void CPUInfo::DumpCPUIDs()
	{
		Cpuid cpuid;

		cpuid.Call(0);
		uint32_t max_std_fn = cpuid.Eax();

		cpuid_std_fn_results_.resize(max_std_fn * 4);
		for (uint32_t i = 0; i < max_std_fn; ++ i)
		{
			cpuid.Call(i);
			cpuid_std_fn_results_[i * 4 + 0] = cpuid.Eax();
			cpuid_std_fn_results_[i * 4 + 1] = cpuid.Ebx();
			cpuid_std_fn_results_[i * 4 + 2] = cpuid.Ecx();
			cpuid_std_fn_results_[i * 4 + 3] = cpuid.Edx();
		}

		cpuid.Call(0x80000000);
		uint32_t max_ext_fn = cpuid.Eax();
		if (max_ext_fn & 0x80000000)
		{
			max_ext_fn -= 0x80000000;
			cpuid_ext_fn_results_.resize(max_ext_fn * 4);
			for (uint32_t i = 0; i < max_ext_fn; ++ i)
			{
				cpuid.Call(i + 0x80000000);
				cpuid_ext_fn_results_[i * 4 + 0] = cpuid.Eax();
				cpuid_ext_fn_results_[i * 4 + 1] = cpuid.Ebx();
				cpuid_ext_fn_results_[i * 4 + 2] = cpuid.Ecx();
				cpuid_ext_fn_results_[i * 4 + 3] = cpuid.Edx();
			}
		}
	}

	unsigned int CPUInfo::CPUIDResult(unsigned int fn, unsigned int index) const
	{
		if (fn < 0x80000000)
		{
			return cpuid_std_fn_results_[fn * 4 + index];
		}
		else
		{
			return cpuid_ext_fn_results_[(fn - 0x80000000) * 4 + index];
		}
	}

	unsigned int CPUInfo::MaxStdFn() const
	{
		return static_cast<unsigned int>(cpuid_std_fn_results_.size() / 4);
	}

	unsigned int CPUInfo::MaxExtFn() const
	{
		return static_cast<unsigned int>(cpuid_ext_fn_results_.size() / 4 + 0x80000000);
	}
}
