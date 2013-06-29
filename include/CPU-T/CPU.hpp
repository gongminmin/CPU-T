/**
 * @file CPU.hpp
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

#ifndef _CPUTSDK_CPU_HPP
#define _CPUTSDK_CPU_HPP

#include <CPU-T/Config.hpp>
#include <vector>
#include <string>

namespace CPUT
{
	class CPUInfo
	{
	public:
		struct TLBInfo
		{
			std::string page;
			int way;
			std::string entry;
		};
		struct CacheInfo
		{
			int size;
			int way;
			int line;
		};

	public:
		enum CPUFeature
		{
			CF_HTT = 1UL << 0,
			CF_CMPXCHG8B = 1UL << 1,
			CF_SYSENTER = 1UL << 2,
			CF_CMOV = 1UL << 3,
			CF_MMX = 1UL << 4,
			CF_3DNow = 1UL << 5,
			CF_3DNowExt = 1UL << 6,
			CF_SSE = 1UL << 7,
			CF_SSE2 = 1UL << 8,
			CF_SSE3 = 1UL << 9,
			CF_SSSE3 = 1UL << 10,
			CF_SSE41 = 1UL << 11,
			CF_SSE42 = 1UL << 12,
			CF_SSE4A = 1UL << 13,
			CF_MisalignedSSE = 1UL << 14,
			CF_X64 = 1UL << 15,
			CF_FMA3 = 1UL << 16,
			CF_CMPXCHG16B = 1UL << 17,
			CF_MOVBE = 1UL << 18,
			CF_POPCNT = 1UL << 19,
			CF_AES = 1UL << 20,
			CF_AVX = 1UL << 21,
			CF_LZCNT = 1UL << 22,
			CF_AVX2 = 1UL << 23,
			CF_FMA4 = 1UL << 24,
			CF_F16C = 1UL << 25
		};

	public:
		CPUInfo();

		char const * CPUName() const
		{
			return cpu_name_.c_str();
		}
		char const * VendorString() const
		{
			return vendor_;
		}
		char const * BrandString() const
		{
			return brand_string_;
		}
		char const * Technology() const
		{
			return tech_.c_str();
		}
		char const * Transistors() const
		{
			return transistors_.c_str();
		}
		char const * CodeName() const
		{
			return codename_.c_str();
		}
		char const * Package() const
		{
			return package_.c_str();
		}
		char const * SerialNumber() const
		{
			return serial_number_;
		}

		bool IsFeatureSupport(CPUFeature feature) const
		{
			return feature_mask_ & feature ? true : false;
		}

		int Type() const
		{
			return type_;
		}
		int Family() const
		{
			return family_;
		}
		int Model() const
		{
			return model_;
		}
		int Stepping() const
		{
			return stepping_;
		}
		
		int Ratio() const
		{
			return ratio_;
		}

		TLBInfo const & L0DataTLB() const
		{
			return l0_data_tlb_;
		}
		TLBInfo const & L1DataTLB() const
		{
			return l1_data_tlb_;
		}
		TLBInfo const & L2DataTLB() const
		{
			return l2_data_tlb_;
		}
		TLBInfo const & CodeTLB() const
		{
			return data_tlb_;
		}
		TLBInfo const & DataTLB() const
		{
			return data_tlb_;
		}

		CacheInfo const & L1CodeCache() const
		{
			return l1_code_cache_;
		}
		CacheInfo const & L1DataCache() const
		{
			return l1_data_cache_;
		}
		CacheInfo const & L2Cache() const
		{
			return l2_cache_;
		}
		CacheInfo const & L3Cache() const
		{
			return l3_cache_;
		}

		int NumHWThreads() const
		{
			return num_hw_threads_;
		}
		int NumCores() const
		{
			return num_cores_;
		}

		void UpdateFrequency();
		unsigned int Frequency() const
		{
			return frequency_;
		}

	private:
#if (defined(CPUT_CPU_X86) || defined(CPUT_CPU_X64)) && !defined(CPUT_PLATFORM_ANDROID)
		void DumpCPUIDs();
		unsigned int CPUIDResult(unsigned int fn, unsigned int index) const;
		unsigned int MaxStdFn() const;
		unsigned int MaxExtFn() const;
#endif

	private:
		std::string cpu_name_;
		char vendor_[13];
		char brand_string_[49];
		char serial_number_[13];
		unsigned int frequency_;
		unsigned __int64 feature_mask_;
		std::string tech_;
		std::string transistors_;
		std::string codename_;
		std::string package_;

		int type_;
		int family_;
		int model_;
		int stepping_;
		int ratio_;
		TLBInfo l0_data_tlb_;
		TLBInfo l1_data_tlb_;
		TLBInfo l2_data_tlb_;
		TLBInfo code_tlb_;
		TLBInfo data_tlb_;
		CacheInfo l1_code_cache_;
		CacheInfo l1_data_cache_;
		CacheInfo l2_cache_;
		CacheInfo l3_cache_;

		int num_hw_threads_;
		int num_cores_;

#if (defined(CPUT_CPU_X86) || defined(CPUT_CPU_X64)) && !defined(CPUT_PLATFORM_ANDROID)
		std::vector<unsigned int> cpuid_std_fn_results_;
		std::vector<unsigned int> cpuid_ext_fn_results_;
#endif
	};
}

#endif		// _CPUTSDK_CPU_HPP
