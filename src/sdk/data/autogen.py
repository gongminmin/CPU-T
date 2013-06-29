#!/usr/bin/env python
#-*- coding: ascii -*-

from __future__ import print_function
try:
	from StringIO import StringIO
except:
	from io import StringIO

class CpuVendorFamilyModelStepping:
	def __init__(self, value, begin, end, name, tech, transistors, core, codename, package):
		if (value):
			self.begin = self.end = int(value, 16);
		else:
			assert len(begin) > 0;
			assert len(end) > 0;
			
			self.begin = int(begin, 16);
			self.end = int(end, 16);

		self.name = name;
		self.tech = tech;
		self.transistors = transistors;
		self.core = core;
		self.codename = codename;
		self.package = package;

class CpuVendorFamilyModel:
	def __init__(self, value, name, tech, transistors, core, codename, package, steppings_tag):
		self.value = value;
		self.name = name;
		self.tech = tech;
		self.transistors = transistors;
		self.core = core;
		self.codename = codename;
		self.package = package;

		self.steppings = []
		if (steppings_tag):
			for stepping in steppings_tag:
				self.steppings.append(CpuVendorFamilyModelStepping(stepping.getAttribute("value"),
					stepping.getAttribute("begin"), stepping.getAttribute("end"),
					stepping.getAttribute("name"), stepping.getAttribute("tech"),
					stepping.getAttribute("transistors"), stepping.getAttribute("core"),
					stepping.getAttribute("codename"), stepping.getAttribute("package")));

	def GenSource(self, source_str):
		if len(self.steppings) > 0:
			source_str.write("\t\t\t\tswitch (stepping)\n");
			source_str.write("\t\t\t\t{\n");
			for stepping in self.steppings:
				for i in range(stepping.begin, stepping.end + 1):
					source_str.write("\t\t\t\tcase %s:\n" % i);
				if len(stepping.name) > 0:
					source_str.write("\t\t\t\t\tname = \"%s\";\n" % stepping.name);
				if len(stepping.tech) > 0:
					source_str.write("\t\t\t\t\ttech = \"%s nm\";\n" % stepping.tech);
				if len(stepping.transistors) > 0:
					source_str.write("\t\t\t\t\ttransistors = \"%s\";\n" % stepping.transistors);
				if len(stepping.codename) > 0:
					source_str.write("\t\t\t\t\tcodename = \"%s\";\n" % stepping.codename);
				if len(stepping.package) > 0:
					source_str.write("\t\t\t\t\tpackage = \"%s\";\n" % stepping.package);
				source_str.write("\t\t\t\t\tbreak;\n\n");
			source_str.write("\t\t\t\tdefault:\n");
			source_str.write("\t\t\t\t\tbreak;\n");
			source_str.write("\t\t\t\t}\n");
		
class CpuVendorFamily:
	def __init__(self, value, name, models_tag):
		self.value = value;
		self.name = name;

		self.models = []
		if (models_tag):
			for model in models_tag:
				self.models.append(CpuVendorFamilyModel(model.getAttribute("value"),
					model.getAttribute("name"), model.getAttribute("tech"),
					model.getAttribute("transistors"), model.getAttribute("core"),
					model.getAttribute("codename"), model.getAttribute("package"),
					model.getElementsByTagName("stepping")));
					
	def GenSource(self, source_str):
		if len(self.models) > 0:
			source_str.write("\t\t\tswitch (model)\n");
			source_str.write("\t\t\t{\n");
			for model in self.models:
				source_str.write("\t\t\tcase 0x%s:\n" % model.value);
				if len(model.name) > 0:
					source_str.write("\t\t\t\tname = \"%s\";\n" % model.name);
				if len(model.tech) > 0:
					source_str.write("\t\t\t\ttech = \"%s nm\";\n" % model.tech);
				if len(model.transistors) > 0:
					source_str.write("\t\t\t\ttransistors = \"%s\";\n" % model.transistors);
				if len(model.codename) > 0:
					source_str.write("\t\t\t\tcodename = \"%s\";\n" % model.codename);
				if len(model.package) > 0:
					source_str.write("\t\t\t\tpackage = \"%s\";\n" % model.package);
				model.GenSource(source_str);
				source_str.write("\t\t\t\tbreak;\n\n");
			source_str.write("\t\t\tdefault:\n");
			source_str.write("\t\t\t\tbreak;\n");
			source_str.write("\t\t\t}\n");

class CpuVendor:
	def __init__(self, vendor, vendor_id, name, families_tag):
		assert len(vendor) > 0;
		assert len(vendor_id) > 0;
		assert len(name) > 0;

		self.vendor = vendor;
		self.vendor_id = vendor_id;
		self.name = name;

		self.families = []
		if (families_tag):
			for family in families_tag:
				self.families.append(CpuVendorFamily(family.getAttribute("value"),
					family.getAttribute("name"), family.getElementsByTagName("model")));

	def GenSource(self, source_str):
		if len(self.families) > 0:
			source_str.write("\t\tswitch (family)\n");
			source_str.write("\t\t{\n");
			for family in self.families:
				source_str.write("\t\tcase 0x%s:\n" % family.value);
				if len(family.name) > 0:
					source_str.write("\t\t\tname = \"%s\";\n" % family.name);
				family.GenSource(source_str);
				source_str.write("\t\t\tbreak;\n\n");
			source_str.write("\t\tdefault:\n");
			source_str.write("\t\t\tbreak;\n");
			source_str.write("\t\t}\n");


class CpuIdentifier:
	def __init__(self, dom):
		self.cpus = [];
		cpus_tag = dom.documentElement;
		if (cpus_tag):
			self.name = cpus_tag.getAttribute("name");
			for cpu in cpus_tag.getElementsByTagName("cpu"):
				self.cpus.append(CpuVendor(cpu.getAttribute("vendor"),
						cpu.getAttribute("id"), cpu.getAttribute("name"),
						cpu.getElementsByTagName("family")));
						
	def GenSource(self, source_str):
		for i in range(0, len(self.cpus)):
			cpu = self.cpus[i];
			if (0 == i):
				source_str.write("\t");
			else:
				source_str.write("\telse ");
			source_str.write("if (\"%s\" == vendor_id)\n" % cpu.vendor_id);
			source_str.write("\t{\n");
			if len(cpu.name) > 0:
				source_str.write("\t\tname = \"%s\";\n" % cpu.name);
			cpu.GenSource(source_str);
			source_str.write("\t\tname = \"%s \" + name;\n" % cpu.vendor);
			source_str.write("\t}\n");


class CacheCodeFamily:
	def __init__(self, value, type, page, size, way, entry, line):
		self.value = value;
		self.type = type;
		self.page = page;
		self.size = size;
		self.way = way;
		self.entry = entry;
		self.line = line;

class CacheCode:
	def __init__(self, value, type, page, size, way, entry, line, families_tag):
		self.value = value;
		self.type = type;
		self.page = page;
		self.size = size;
		self.way = way;
		self.entry = entry;
		self.line = line;

		self.families = [];
		if (families_tag):
			for family in families_tag:
				self.families.append(CacheCodeFamily(family.getAttribute("value"),
						family.getAttribute("type"), family.getAttribute("page"), family.getAttribute("size"),
						family.getAttribute("way"), family.getAttribute("entry"), family.getAttribute("line")));
					
	def GenSource(self, source_str):
		if len(self.families) > 0:
			source_str.write("\t\tswitch (family)\n");
			source_str.write("\t\t{\n");
			for family in self.families:
				source_str.write("\t\tcase 0x%s:\n" % family.value);
				if len(family.type) > 0:
					source_str.write("\t\t\ttype = \"%s\";\n" % family.type);
				if len(family.page) > 0:
					source_str.write("\t\t\tpage = \"%s\";\n" % family.page);
				if len(family.size) > 0:
					source_str.write("\t\t\tsize = %s;\n" % family.size);
				if len(family.way) > 0:
					source_str.write("\t\t\tway = 0x%s;\n" % family.way);
				if len(family.entry) > 0:
					source_str.write("\t\t\tentry = %s;\n" % family.entry);
				if len(family.line) > 0:
					source_str.write("\t\t\tline = %s;\n" % family.line);
				source_str.write("\t\t\tbreak;\n\n");
			source_str.write("\t\tdefault:\n");
			source_str.write("\t\t\tbreak;\n");
			source_str.write("\t\t}\n");
			
class CacheIdentifier:
	def __init__(self, dom):
		self.codes = [];
		cache_tag = dom.documentElement;
		if (cache_tag):
			for code in cache_tag.getElementsByTagName("code"):
				self.codes.append(CacheCode(code.getAttribute("value"),
						code.getAttribute("type"), code.getAttribute("page"), code.getAttribute("size"),
						code.getAttribute("way"), code.getAttribute("entry"), code.getAttribute("line"),
						code.getElementsByTagName("family")));
						
	def GenSource(self, source_str):
		source_str.write("\tswitch (code)\n");
		source_str.write("\t{\n");
		for code in self.codes:
			source_str.write("\tcase 0x%s:\n" % code.value)
			if len(code.type) > 0:
				source_str.write("\t\ttype = \"%s\";\n" % code.type);
			if len(code.page) > 0:
				source_str.write("\t\tpage = \"%s\";\n" % code.page);
			if len(code.size) > 0:
				source_str.write("\t\tsize = %s;\n" % code.size);
			if len(code.way) > 0:
				source_str.write("\t\tway = 0x%s;\n" % code.way);
			if len(code.entry) > 0:
				source_str.write("\t\tentry = \"%s\";\n" % code.entry);
			if len(code.line) > 0:
				source_str.write("\t\tline = %s;\n" % code.line);
			code.GenSource(source_str);
			source_str.write("\t\tbreak;\n\n");
		source_str.write("\t\tdefault:\n");
		source_str.write("\t\t\tbreak;\n");
		source_str.write("\t}\n");


def CreateCpuIdentifierSource(cpu_identifier):
	source_str = StringIO()

	source_str.write("//AUTO-GENERATED BY autogen.py. DON'T EDIT THIS FILE.\n\n");

	source_str.write("void CPUIdentify(std::string const & vendor_id, int family, int model, int stepping,\n");
	source_str.write("\t\tstd::string& name, std::string& tech, std::string& transistors, std::string& codename, std::string& package)\n");
	source_str.write("{\n");
	if len(cpu_identifier.name) > 0:
		source_str.write("\tname = \"%s\";\n" % cpu_identifier.name);
	cpu_identifier.GenSource(source_str);
	source_str.write("}\n");

	try:
		cur_source_file = open("../CPUIdentifier.cpp", "r")
		cur_source_str = cur_source_file.read()
		cur_source_file.close()
	except:
		cur_source_str = ""
	new_source_str = source_str.getvalue()
	if new_source_str != cur_source_str:
		source_file = open("../CPUIdentifier.cpp", "w")
		source_file.write(new_source_str)
		source_file.close()
		print("CPUIdentifier.cpp has been updated")
	else:
		print("No change detected. Skip CPUIdentifier.cpp")

def CreateCacheIdentifierSource(cache_identifier):
	source_str = StringIO()

	source_str.write("//AUTO-GENERATED BY autogen.py. DON'T EDIT THIS FILE.\n\n");

	source_str.write("void CacheIdentify(int code, int family,\n");
	source_str.write("\t\tstd::string& type, std::string& page, int& size, int& way, std::string& entry, int& line)\n");
	source_str.write("{\n");
	cache_identifier.GenSource(source_str);
	source_str.write("}\n");

	try:
		cur_source_file = open("../CacheIdentifier.cpp", "r")
		cur_source_str = cur_source_file.read()
		cur_source_file.close()
	except:
		cur_source_str = ""
	new_source_str = source_str.getvalue()
	if new_source_str != cur_source_str:
		source_file = open("../CacheIdentifier.cpp", "w")
		source_file.write(new_source_str)
		source_file.close()
		print("CacheIdentifier.cpp has been updated")
	else:
		print("No change detected. Skip CacheIdentifier.cpp")

def AutoGenCpuIdentifier():
	from xml.dom.minidom import parse
	CreateCpuIdentifierSource(CpuIdentifier(parse("cpus.xml")));
	CreateCacheIdentifierSource(CacheIdentifier(parse("caches.xml")));

if __name__ == "__main__":
	AutoGenCpuIdentifier();
