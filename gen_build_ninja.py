# -*- coding: utf-8 -*-
###################################################################################
# Copyright (c) 2013, Wictor Lund. All rights reserved.                           #
# Copyright (c) 2013, Åbo Akademi University. All rights reserved.                #
#                                                                                 #
# Redistribution and use in source and binary forms, with or without              #
# modification, are permitted provided that the following conditions are met:     #
#      * Redistributions of source code must retain the above copyright           #
#        notice, this list of conditions and the following disclaimer.            #
#      * Redistributions in binary form must reproduce the above copyright        #
#        notice, this list of conditions and the following disclaimer in the      #
#        documentation and/or other materials provided with the distribution.     #
#      * Neither the name of the Åbo Akademi University nor the                   #
#        names of its contributors may be used to endorse or promote products     #
#        derived from this software without specific prior written permission.    #
#                                                                                 #
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND #
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED   #
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          #
# DISCLAIMED. IN NO EVENT SHALL ÅBO AKADEMI UNIVERSITY BE LIABLE FOR ANY          #
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES      #
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;    #
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND     #
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT      #
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS   #
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                    #
###################################################################################

import sys
import ninja_syntax
import itertools
import glob

################
# Code section #
################

class NinjaFile(str):
    def get_basename(self):
        return self.split("/")[-1].split(".")[0]
    def get_object_file(self, config):
        dirprefix = ""
        for s in self.split('/')[0:-1]:
            dirprefix += s.lower() + "_"
        return builddir + dirprefix + self.get_basename() + \
            ("-" + config['name'] if config.has_key('name') else '')  + ".o"
    def get_object_files(self, config):
        return NinjaSet([self.get_object_file(config)])
    def write_builds(self, n, config):
        config.preprocess()
        n.build(outputs   = self.get_object_file(config),
                rule      = "cc",
                variables = config,
                inputs    = self,
                implicit  = config['implicit'] if config.has_key('implicit') else [])
    def get_flattened(self):
        return NinjaSet([self])
    def get_flat_list(self):
        return [self]

class NinjaSet(set):
    def get_real_config(self, config):
        if hasattr(self, 'config'):
            config = self.config + config
        return config
    def get_object_files(self, config):
        config = self.get_real_config(config)
        ret = NinjaSet()
        for i in self:
            ret = ret.union(i.get_object_files(config))
        return ret
    def write_builds(self, n, config):
        config = self.get_real_config(config)
        for i in self:
            i.write_builds(n, config)
    def __add__(self, op):
        ret = NinjaSet()
        ret.add(self)
        ret.add(op)
        return ret
    def get_flattened(self):
        return NinjaSet(itertools.chain(*map(lambda l: l.get_flattened(), self)))
    def get_flat_list(self):
        return list(set(itertools.chain(*map(lambda l: l.get_flat_list(), self))))
    def __hash__(self):
        ret = 0
        for i in self:
            ret += i.__hash__()
        return ret

class NinjaConfig(dict):
    def __add__(self, op):
        ret = NinjaConfig(self.copy())
        ret.update(op)
        for k in self:
            if op.has_key(k):
                ret[k] = self[k] + op[k]
        return ret
    def preprocess(self):
        includedir_args = get_include_args(self['includedirs']) if self.has_key('includedirs') else ""
        cflags_args = ""
        if self.has_key('cflags'):
            for arg in self['cflags'][0:-1]:
                cflags_args += arg + " "
            if len(self['cflags']) > 0:
                cflags_args += self['cflags'][-1]
        new_config = NinjaConfig({'includedir_args': includedir_args,
                                  'cflags_args': cflags_args})
        self.update(new_config)
        return self

def get_ninja_set_of_files(flist, config = {}):
    ret = NinjaSet(map(lambda f: NinjaFile(f), flist))
    if config != {}:
        ret.config = config
    return ret

def get_sets_in_dict(dictionary):
    return NinjaSet(itertools.chain(map(lambda c: dictionary[c], dictionary)))

def get_include_args(dirs):
    ret = ""
    for d in dirs:
        ret += "-I" + d + " "
    return ret

#####################
# Parameter section #
#####################

builddir = "./build/"
bindir   = "./bin/"

sourcedir = "."

devkitdir = "/export/home/aton4/wlund/bin/devkitARM/bin/"

#n = ninja_syntax.Writer(sys.stdout)
fout = open("build.ninja", "w")
n = ninja_syntax.Writer(fout)

common_includedirs = \
    [sourcedir,
     sourcedir + "/System/umm",
     sourcedir + "/libdwarf",
     sourcedir + "/Source/include",
     sourcedir + "/Source/portable/GCC/ARM_Cortex-A9"]

n.variable(key="cc", value=devkitdir + "arm-eabi-gcc")
n.variable(key="ld", value=devkitdir + "arm-eabi-ld")
n.variable(key="objcopy", value=devkitdir + "arm-eabi-objcopy")
n.variable(key="mkimage", value="mkimage")
n.variable(key="cloc", value="cloc")
n.variable(key="cscope", value="cscope")
n.variable(key="image_address", value="0x10000")

default_cflags = ["-Wall", "-fmessage-length=0", "-mcpu=cortex-a9",
                  "-g3", "-Werror", "-fno-builtin-printf",
                  "-fPIC"]

######################
# Source set section #
######################

freertos_files = \
    get_ninja_set_of_files(
    map(lambda f: "Source/" + f,
        ["croutine.c", "list.c", "queue.c",
         "tasks.c", "timers.c", "portable/GCC/ARM_Cortex-A9/port.c"]))

freertos_memmang_files = \
    get_ninja_set_of_files(
    map(lambda n: NinjaFile("Source/portable/MemMang/heap_%i.c" % n), [2,3,4]))

applications = { 'simple': get_ninja_set_of_files(['App/app_startup.S', 'App/simple.c']),
                 'writer': get_ninja_set_of_files(['App/app_startup.S', 'App/writer.c']),
                 'reader': get_ninja_set_of_files(['App/app_startup.S', 'App/reader.c']),
                 'rtuappv1': get_ninja_set_of_files(['App/app_startup.S', 'App/rtu_appv1.c']),
                 'rtuappv2': get_ninja_set_of_files(['App/app_startup.S', 'App/rtu_appv2.c']),
                 'tunnel': get_ninja_set_of_files(['App/app_startup.S', 'App/effects/tunnel.c',
                                                   'App/effects/tunnel_effect.c', 'App/effects/Utils.c']),
                 'field': get_ninja_set_of_files(['App/app_startup.S', 'App/effects/field.c',
                                                  'App/effects/field_effect.c', 'App/effects/Utils.c'])}

vexpress_vm_boot_files = get_ninja_set_of_files(
    map(lambda f: "System/arch/vexpress_vm/" + f,
        ["boot/loader.c", "boot/loader-startup.S"]))
vexpress_novm_boot_files = get_ninja_set_of_files(
    map(lambda f: "System/arch/vexpress_novm/" + f,
        ["boot/startup.S"]))


libdwarf_files = get_ninja_set_of_files(
    glob.glob('libdwarf/*.c'))

system_utility_files = get_ninja_set_of_files(
    ['System/printf-stdarg.c', 'System/serial.c', 'System/pl011.c',
     'System/pl111.c', 'System/umm/umm_malloc.c', 'System/qsort.c'])

system_files = get_ninja_set_of_files(
    ['System/task_manager.c', 'System/pointer_tracer.c', 'System/linker.c',
     'System/migrator.c', 'System/dwarfif.c', 'System/system_util.c'])

##################
# Config section #
##################

configs = \
    [NinjaConfig(
        {'name': "rtudemo",
         'includedirs': common_includedirs + ["./libdwarf", "./System/config/rtudemo/include"],
         'cflags': ["-O1"] + default_cflags,
         'image_address': '0x10000'
         }),
     
     NinjaConfig(
            {'name': "taskmigr",
             'includedirs': common_includedirs + \
                 ["./System/config/taskmigr/include"],
             'cflags': ["-O2"] + default_cflags,
             'image_address': '0x60100000'
             })
     ]

map(lambda config: config.preprocess(), configs)

config_source_files = \
    { 'rtudemo': get_ninja_set_of_files(
        ['System/arch/vexpress_novm/main.c',
         'Source/portable/MemMang/heap_3.c']) + \
          system_files + \
          system_utility_files + \
          freertos_files + \
          libdwarf_files + \
          vexpress_novm_boot_files,

      'taskmigr': get_ninja_set_of_files(
        ['System/arch/vexpress_vm/main.c',
         'System/arch/vexpress_vm/kernel-startup.S',
         'System/arch/vexpress_vm/setup_vm.c',
         'Source/portable/MemMang/heap_4.c']) + \
          freertos_files + \
          vexpress_vm_boot_files + \
          (system_files - NinjaSet(["System/dwarfif.c", "System/pointer_tracer.c"])) + \
          system_utility_files
      }

################
# Rule section #
################

contributed_files = \
    get_sets_in_dict(applications) + vexpress_vm_boot_files + vexpress_novm_boot_files + \
    system_utility_files + system_files + get_sets_in_dict(config_source_files) + \
    get_ninja_set_of_files(["gen_build_ninja.py"])


all_files = \
    freertos_files + freertos_memmang_files + get_sets_in_dict(applications) + \
    vexpress_vm_boot_files + vexpress_novm_boot_files + libdwarf_files + \
    system_utility_files + system_utility_files + system_files + \
    get_sets_in_dict(config_source_files)

includedirs = common_includedirs + \
    list(itertools.chain(*map(lambda c: c['includedirs'] if c.has_key('includedirs') else [],
                              configs)))
includedirs = list(set(includedirs))

n.rule(name = "cc",
       command = "$cc -MMD -MT $out -MF $out.d -c -gdwarf-3 $includedir_args $cflags_args $app_cflags $in -o $out",
       description = "CC $out",
       depfile = "$out.d")

n.rule(name = "link",
       command = "$cc $ldflags $shared -o $out $in $libs",
       description = "LINK $out")

n.rule(name = "objcopy",
       command = "$objcopy $ocflags $in $out",
       description = "OBJCOPY $in")

n.rule(name = "mkimage",
       command = "$mkimage -A arm -O linux -T kernel -C none -a $image_address -e $image_address -d $in -n FreeRTOS $out",
       description = "MKIMAGE $in")

n.rule(name = "app_ld",
       command = """hexdump -v -e '"BYTE(0x" 1/1 "%02X" ")\\n"' $in > $out""",
       description = "APP LD FILE")

n.rule(name = "cloc",
       command = "$cloc $clocflags --report-file=$out $in",
       description = "CLOC $out")

n.rule(name = "cscope",
       command = "$cscope -b -f $out " + get_include_args(includedirs) + " $in",
       description = "CSCOPE $out")

n.rule(name = "m4",
       command = "m4 $m4flags $in > $out",
       description = "M4 $out")

##########################
# Build object files     #
##########################

for c in configs:
    s = config_source_files[c['name']]
    s.write_builds(n, c)
    s = NinjaSet()
    app_c = c + NinjaConfig({'cflags': ['-DIN_APPTASK']})
    s = reduce(lambda s, a: s.union(applications[a]), applications, NinjaSet())
    for a in applications:
        #s = s.union(applications[a])
        app_s = applications[a]
        elffile = bindir + a + "-" + c['name'] + ".elf"
        ldfile  = builddir + a + "-" + c['name'] + ".ld"
        n.build(outputs   = elffile,
                rule      = "link",
                inputs    = list(app_s.get_object_files(app_c)),
                variables = {'ldflags': '-nostartfiles -TApp/app.ld -mcpu=cortex-a9 -g3 -fPIC -gdwarf-3 -shared' },
                implicit  = "App/app.ld")
        n.build(outputs   = ldfile,
                rule      = "app_ld",
                inputs    = elffile)
    s.write_builds(n, app_c)
    n.build(outputs   = builddir + "applications-" + c['name'] + ".ld",
            rule      = "m4",
            inputs    = "System/applications.ld.m4",
            variables = {'m4flags': '-DCONFIG=' + c['name']})

#######################
# Link configurations #
#######################

 ############
 # RTU Demo #
 ############

def gen_step_build(name, inputs, implicit, ldfiles, config):
    i = 0
    for ldf in ldfiles[0:-1]:
        elffile    = builddir + name + "." + str(i) + ".elf"
        symelffile = builddir + name + "." + str(i) + ".sym.elf"
        ldfile     = builddir + name + "." + str(i) + ".ld"
        n.build(outputs   = elffile,
                rule      = "link",
                inputs    = list(inputs.get_object_files(config)),
                variables = config + NinjaConfig({'ldflags': '-nostartfiles -Wl,-T,' + ldf + ' -mcpu=cortex-a9 -g3 -gdwarf-3'}),
                implicit  = [builddir + name + "." + str(i - 1) + ".ld", ldf] if i > 0 else [ldf] + implicit)
        n.build(outputs   = symelffile,
                rule      = "objcopy",
                inputs    = elffile,
                variables = config + NinjaConfig({'ocflags':'--extract-symbol'}))
        n.build(outputs   = ldfile,
                rule      = "app_ld",
                inputs    = symelffile)
        i += 1
    lastnldfile       = builddir + name + "." + str(i - 1) + ".ld"
    n.build(outputs   = bindir + name + ".elf",
            rule      = "link",
            inputs    = list(inputs.get_object_files(config)),
            variables = config + NinjaConfig({'ldflags': '-nostartfiles -Wl,-T,' + ldfiles[i] + ' -mcpu=cortex-a9 -g3 -gdwarf-3'}),
            implicit  = [lastnldfile] + implicit)
    n.build(outputs   = bindir + name + ".bin",
            rule      = "objcopy",
            inputs    = bindir + name + ".elf",
            variables = config + NinjaConfig({'ocflags': '-O binary'}))
    n.build(outputs   = bindir + name + ".uimg",
            rule      = "mkimage",
            inputs    = bindir + name + ".bin",
            variables = config)

c = configs[0]

gen_step_build("rtudemo", config_source_files['rtudemo'],
               map(lambda f: builddir + f + "-rtudemo.ld", applications) + [builddir + "applications-rtudemo.ld"],
               ["System/rtudemo.0.ld", "System/rtudemo.1.ld", "System/rtudemo.2.ld"], c)

 #######################
 # Task migration demo #
 #######################

c = configs[1]

n.build(outputs   = bindir + "kernel-taskmigr.elf",
        rule      = "link",
        inputs    = list((config_source_files['taskmigr'].get_flattened() -
                          vexpress_vm_boot_files).get_object_files(c)) +  \
            ['./WittCore2Core.a'],
        variables = c + NinjaConfig({'ldflags': '-nostartfiles -fPIC -Wl,-T,System/arch/vexpress_vm/kernel.ld -mcpu=cortex-a9 -g3 -gdwarf-3', 'libs': '-lm'}),
        implicit  = map(lambda f: builddir + f + "-taskmigr.ld", list(applications)) + \
            ["System/arch/vexpress_vm/kernel.ld", builddir + "applications-taskmigr.ld"])
n.build(outputs   = builddir + "kernel-taskmigr.ld",
        rule      = "app_ld",
        inputs    = bindir + "kernel-taskmigr.elf")
n.build(outputs = bindir + "boot-taskmigr.elf",
        rule = "link",
        inputs = list((vexpress_vm_boot_files + system_utility_files).get_object_files(c)),
        variables = c + NinjaConfig({'ldflags': '-nostartfiles -fPIC -Wl,-T,System/arch/vexpress_vm/boot/loader.ld -mcpu=cortex-a9 -g3 -gdwarf-3'}),
        implicit = ["System/arch/vexpress_vm/boot/loader.ld", builddir + "kernel-taskmigr.ld"])
n.build(outputs   = bindir + "boot-taskmigr.bin",
        rule      = "objcopy",
        inputs    = bindir + "boot-taskmigr.elf",
        variables = c + NinjaConfig({'ocflags': '-O binary'}))
n.build(outputs   = bindir + "boot-taskmigr.uimg",
        rule      = "mkimage",
        inputs    = bindir + "boot-taskmigr.bin",
        variables = c)


################
# Misc. builds #
################

n.build(outputs = "cloc_report.log",
        rule = "cloc",
        inputs = contributed_files.get_flat_list())
n.build(outputs = "cscope.out",
        rule = "cscope",
        inputs = all_files.get_flat_list())

n.default([
        bindir + "rtudemo.uimg",
        "cloc_report.log",
        "cscope.out",
        ])
