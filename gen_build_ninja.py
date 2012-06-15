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
import glob
import ninja_syntax

def get_basename(filename):
    return filename.split("/")[-1].split(".")[0]

def get_object_file(filename, config = '', object_postfix = '', in_app = False):
    dirprefix = ""
    if config != '':
        if not config in configs:
            raise Exception("No such configuraton \"" + config + "\".")
    for s in filename.split('/')[0:-1]:
        dirprefix += s.lower() + "_"
    return builddir + dirprefix + ("app_" if in_app else "") + \
        get_basename(filename) + ("-" + config if config != '' else '') + object_postfix + ".o"

def get_include_args(dirs):
    ret = ""
    for d in dirs:
        ret += "-I" + d + " "
    return ret

builddir = "./build/"
bindir   = "./bin/"

fout = open("build.ninja","w")
#n = ninja_syntax.Writer(sys.stdout)
n = ninja_syntax.Writer(fout)

devkitdir = "/export/home/aton4/wlund/bin/devkitARM/bin/"
#devkitdir = "/tmp/devkitARM/bin/"
sourcedir = "."

configs = ["vm", "no_vm"]

includedirs = [sourcedir,
               sourcedir + "/System/umm",
               sourcedir + "/Source/include",
               sourcedir + "/Source/portable/GCC/ARM_Cortex-A9",
               sourcedir + "/libdwarf"]

n.variable(key="cc", value=devkitdir + "arm-eabi-gcc")
n.variable(key="ld", value=devkitdir + "arm-eabi-ld")
n.variable(key="objcopy", value=devkitdir + "arm-eabi-objcopy")
n.variable(key="mkimage", value="mkimage")
n.variable(key="cloc", value="cloc")
n.variable(key="cscope", value="cscope")
n.variable(key="image_address", value="0x10000")

# -Werror

n.variable(key="include_dirs",
           value=get_include_args(includedirs))
n.variable(key="cflags",
           value="-O0 -Wall -fmessage-length=0 " +
                 "-mcpu=cortex-a9 -g3 -Werror -fno-builtin-printf -fPIC")

n.rule(name = "cc",
       command = "$cc -MMD -MT $out -MF $out.d -c -gdwarf-3 $include_dirs $cflags $app_cflags $in -o $out",
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

freertos_files = ["Source/croutine.c",
                  "Source/list.c",
                  "Source/queue.c",
                  "Source/tasks.c",
                  "Source/timers.c",
                  "Source/portable/GCC/ARM_Cortex-A9/port.c",
                  ]

freertos_memmang_files = map(lambda n: "Source/portable/MemMang/heap_%i.c" % n, [2,3,4])

applications = { 'simple': ['App/app_startup.S', 'App/simple.c'],
                 'writer': ['App/app_startup.S', 'App/writer.c'],
                 'reader': ['App/app_startup.S', 'App/reader.c'],
                 'rtuappv1': ['App/app_startup.S', 'App/rtu_appv1.c'],
                 'rtuappv2': ['App/app_startup.S', 'App/rtu_appv2.c'] }

vexpress_boot_files   = map(lambda f: "System/arch/vexpress_vm/" + f,
                            ["boot/loader.c", "boot/loader-startup.S"])
vexpress_kernel_files = map(lambda f: "System/arch/vexpress_vm/" + f,
                            ["main.c", "kernel-startup.S", "setup_vm.c"])

rtu_kernel_files = ['App/startup.S', 'System/main.c']


libdwarf_files = glob.glob('libdwarf/*.c')

system_utility_files = \
    ['System/printf-stdarg.c', 'System/serial.c', 'System/pl011.c',
     'System/umm/umm_malloc.c', 'System/qsort.c']

system_files = \
    libdwarf_files + \
    freertos_files + \
    ['App/startup.S', 'System/main.c', 'System/task_manager.c',
     'System/pointer_tracer.c', 'System/linker.c', 'System/migrator.c',
     'System/dwarfif.c', 'System/system_util.c'] + \
     system_utility_files

contributed_files = \
    ['System/applications.h', 'System/dwarfif.c', 'System/dwarfif.h',
     'System/linker.c', 'System/linker.h', 'System/main.c',
     'System/migrator.c', 'System/migrator.h', 'System/pointer_tracer.c',
     'System/pointer_tracer.h', 'System/system_config.h', 'System/task_manager.c',
     'System/task_manager.h', 'System/types.h', 'System/system_util.c',
     'System/system_util.h', 'gen_build_ninja.py']

app_fs = set()
for a in applications:
    for f in applications[a]:
        app_fs.add(f)

fs = set()
for f in system_files + freertos_memmang_files + vexpress_boot_files + vexpress_kernel_files:
    fs.add(f)

for c in configs:
    for f in list(app_fs):
        n.build(outputs = get_object_file(f, in_app = True, config=c),
                rule = "cc",
                variables = {'app_cflags': "-DIN_APPTASK",
                             'include_dirs': get_include_args(includedirs + ["System/config/" + c])},
                inputs = f)
    for f in list(fs):
        n.build(outputs = get_object_file(f, config=c),
                rule = "cc",
                variables = {'include_dirs': get_include_args(includedirs + ["System/config/" + c])},
                inputs = f)

def gen_step_build(name, inputs, implicit, ldfiles):
    i = 0
    for ldf in ldfiles[0:-1]:
        elffile    = builddir + name + "." + str(i) + ".elf"
        symelffile = builddir + name + "." + str(i) + ".sym.elf"
        ldfile     = builddir + name + "." + str(i) + ".ld"
        n.build(outputs   = elffile,
                rule      = "link",
                inputs    = inputs,
                variables = {'ldflags': '-nostartfiles -Wl,-T,' + ldf + ' -mcpu=cortex-a9 -g3 -gdwarf-3'},
                implicit  = [builddir + name + "." + str(i - 1) + ".ld", ldf] if i > 0 else [ldf] + implicit)
        n.build(outputs   = symelffile,
                rule      = "objcopy",
                inputs    = elffile,
                variables = {'ocflags':'--extract-symbol'})
        n.build(outputs   = ldfile,
                rule      = "app_ld",
                inputs    = symelffile)
        i += 1
    lastnldfile       = builddir + name + "." + str(i - 1) + ".ld"
    n.build(outputs   = bindir + name + ".elf",
            rule      = "link",
            inputs    = inputs,
            variables = {'ldflags': '-nostartfiles -Wl,-T,' + ldfiles[i] + ' -mcpu=cortex-a9 -g3 -gdwarf-3'},
            implicit  = [lastnldfile] + implicit)
    n.build(outputs   = bindir + name + ".bin",
            rule      = "objcopy",
            inputs    = bindir + name + ".elf",
            variables = {'ocflags': '-O binary'})
    n.build(outputs   = bindir + name + ".uimg",
            rule      = "mkimage",
            inputs    = bindir + name + ".bin")


gen_step_build("system", map(lambda f: get_object_file(f, config="no_vm"),
                             system_files + ["Source/portable/MemMang/heap_3.c"]),
               map(lambda f: builddir + f + "-no_vm.ld", applications) + [builddir + "applications-no_vm.ld"],
               ["System/system.0.ld", "System/system.1.ld", "System/system.2.ld"])

n.build(outputs   = bindir + "vexpress-kernel.elf",
        rule      = "link",
        inputs    = map(lambda f: get_object_file(f, config="vm"), freertos_files + ["Source/portable/MemMang/heap_4.c"] +
                        vexpress_kernel_files + system_utility_files),
        variables = {'ldflags': '-nostartfiles -fPIC -Wl,-T,System/arch/vexpress_vm/kernel.ld -mcpu=cortex-a9 -g3 -gdwarf-3'},
        implicit  = map(lambda f: builddir + f + "-vm.ld", applications) +
        ["System/arch/vexpress/kernel.ld", builddir + "applications-vm.ld"])
n.build(outputs   = builddir + "vexpress-kernel.ld",
        rule      = "app_ld",
        inputs    = bindir + "vexpress-kernel.elf")

n.build(outputs   = bindir + "vexpress-boot.elf",
        rule      = "link",
        inputs    = map(lambda f: get_object_file(f, config="vm"), vexpress_boot_files + system_utility_files),
        variables = {'ldflags': '-nostartfiles -fPIC -Wl,-T,System/arch/vexpress_vm/boot/loader.ld -mcpu=cortex-a9 -g3 -gdwarf-3'},
        implicit  = ["System/arch/vexpress_vm/boot/loader.ld", builddir + "vexpress-kernel.ld"])
n.build(outputs   = bindir + "vexpress-boot.bin",
        rule      = "objcopy",
        inputs    = bindir + "vexpress-boot.elf",
        variables = {'ocflags': '-O binary'})
n.build(outputs   = bindir + "vexpress-boot.uimg",
        rule      = "mkimage",
        inputs    = bindir + "vexpress-boot.bin",
        variables = {'image_address': '0x60100000'})


n.build(outputs = "cloc_report.log",
        rule = "cloc",
        inputs = contributed_files)

n.build(outputs = "cscope.out",
        rule = "cscope",
        inputs = list(set.union(app_fs,fs)))

for c in configs:
    n.build(outputs   = builddir + "applications-" + c + ".ld",
            rule      = "m4",
            inputs    = "System/applications.ld.m4",
            variables = {'m4flags': '-DCONFIG=' + c})
    for a in applications:
        elffile           = bindir + a + "-" + c + ".elf"
        ldfile            = builddir + a + "-" + c + ".ld"
        n.build(outputs   = elffile,
                rule      = "link",
                inputs    = map(lambda f: get_object_file(f, in_app = True, config=c), applications[a]),
                variables = {'ldflags': '-nostartfiles -TApp/app.ld -mcpu=cortex-a9 -g3 -fPIC -gdwarf-3 -shared' },
                implicit  = "App/app.ld")
        n.build(outputs   = ldfile,
                rule      = "app_ld",
                inputs    = elffile)

n.default([
        bindir + "system.uimg",
        "cloc_report.log",
        "cscope.out",
        ])
