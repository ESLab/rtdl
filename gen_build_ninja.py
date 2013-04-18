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

def get_basename(filename):
    return filename.split("/")[-1].split(".")[0]

def get_object_file(filename, object_postfix = ''):
    return builddir + get_basename(filename) + object_postfix + ".o"

def get_include_args(dirs):
    ret = ""
    for d in dirs:
        ret += "-I" + d + " "
    return ret

builddir = "./build/"

fout = open("build.ninja","w")
#n = ninja_syntax.Writer(sys.stdout)
n = ninja_syntax.Writer(fout)

devkitdir = "/export/home/aton4/wlund/bin/devkitARM/bin/"
sourcedir = "."

# includedirs = [sourcedir + "/Demo",
#                sourcedir + "/Demo/Common/include",
#                sourcedir + "/Source/include",
#                sourcedir + "/Source/portable/GCC/ARM_Cortex-A9"]

includedirs = [sourcedir + "/App",
               sourcedir + "/System",
               sourcedir + "/Source/include",
               sourcedir + "/Source/portable/GCC/ARM_Cortex-A9"]

n.variable(key="cc", value="arm-eabi-gcc")
n.variable(key="ld", value="arm-eabi-ld")
n.variable(key="objcopy", value="arm-eabi-objcopy")
n.variable(key="mkimage", value="mkimage")
n.variable(key="cflags", 
           value=get_include_args(includedirs) + 
           "-O0 -Wall -Werror -fmessage-length=0 " +
           "-mcpu=cortex-a9 -g3 -fno-builtin-printf -fPIC")

n.rule(name = "cc",
       command = "$cc -MMD -MT $out -MF $out.d -c -gdwarf-3 $cflags $in -o $out",
       description = "CC $out",
       depfile = builddir + "$out.d")

n.rule(name = "link",
       command = "$cc $ldflags  $shared -o $out $in $libs",
       description = "LINK $out")

n.rule(name = "objcopy",
       command = "$objcopy $ocflags $in $out",
       description = "OBJCOPY $in")

n.rule(name = "mkimage",
       command = "$mkimage -A arm -O linux -T kernel -C none -a 0x10000 -e 0x10000 -d $in -n FreeRTOS $out",
       description = "MKIMAGE $in")

n.rule(name = "app_ld",
       command = """hexdump -v -e '"BYTE(0x" 1/1 "%02X" ")\\n"' $in > $out""",
       description = "APP LD FILE")

freertos_files = ["Source/croutine.c",
                  "Source/list.c",
                  "Source/queue.c",
                  "Source/tasks.c",
                  "Source/timers.c",
                  "Source/portable/GCC/ARM_Cortex-A9/port.c",
                  "Source/portable/MemMang/heap_2.c"]

# applications = { 'simple': ['App/simple.c', 'App/startup.S', 'App/printf-stdarg.c', 'App/pl011.c'],
#                  'uart_out': ['App/uart.c', 'App/uart_out.c' ] }

applications = { 'simple': ['App/app_startup.S', 'App/simple.c'],
                 'writer': ['App/app_startup.S', 'App/writer.c'],
                 'reader': ['App/app_startup.S', 'App/reader.c'],
                 'rtuappv1': ['App/app_startup.S', 'App/rtu_appv1.c'],
                 'rtuappv2': ['App/app_startup.S', 'App/rtu_appv2.c'] }

#system_files = freertos_files + ['App/startup.S', 'App/printf-stdarg.c', 'App/serial.c', 'App/pl011.c']
system_files = freertos_files + ['App/startup.S', 'System/main.c', 'System/linker.c',
                                 'System/migrator.c', 'System/printf-stdarg.c', 'System/serial.c', 
                                 'System/pl011.c']

fs = set()
for a in applications:
    for f in applications[a]:
        fs.add(f)
for f in system_files:
    fs.add(f)

for f in list(fs):
    n.build(outputs = get_object_file(f),
            rule = "cc",
            inputs = f)

# systemso = "system.so"
# n.build(outputs = systemso,
#         rule = "link",
#         inputs = map(lambda f: get_object_file(f), system_files),
#         variables = {'shared': '-shared',
#                      'ldflags': '-nostartfiles -fPIC -Wl,-T,App/plain.ld -mcpu=cortex-a9'},
#         implicit = 'App/plain.ld')
#                     'ldflags': '-nostartfiles -Wl,-T,App/plain.ld -mcpu=cortex-a9 -g3 -gdwarf-3'})

systemelf = "system.elf"
systembin = "system.bin"
systemuimg = "system.uimg"
n.build(outputs = systemelf,
        rule = "link",
        inputs = map(lambda f: get_object_file(f), system_files),
        variables = {'ldflags': '-nostartfiles -Wl,-T,System/system.ld -mcpu=cortex-a9 -g3 -gdwarf-3'},
        implicit = map(lambda f: f + ".ld", applications) + ["System/system.ld", "System/applications.ld"])
n.build(outputs = systembin,
        rule = "objcopy",
        inputs = systemelf,
        variables = {'ocflags': '-O binary'})
n.build(outputs = systemuimg,
        rule = "mkimage",
        inputs = systembin)

systemext_elf = "system_ext_.elf"
systemextelf = "system_ext.elf"
systemextbin = "system_ext.bin"
systemextuimg = "system_ext.uimg"

n.build(outputs = "system_sym.elf",
        rule = "objcopy",
        inputs = systemext_elf,
        variables = {'ocflags':'--extract-symbol'})
n.build(outputs = "system_sym_.elf",
        rule = "objcopy",
        inputs = systemelf,
        variables = {'ocflags':'--extract-symbol'})

n.build(outputs = "system_elf.ld",
        rule = "app_ld",
        inputs = "system_sym.elf")
n.build(outputs = "system_elf_.ld",
        rule = "app_ld",
        inputs = "system_sym_.elf")

n.build(outputs = systemextelf,
        rule = "link",
        inputs = map(lambda f: get_object_file(f), system_files),
        variables = {'ldflags': '-nostartfiles -Wl,-T,System/system_ext.ld -mcpu=cortex-a9 -g3 -gdwarf-3'},
        implicit = ["System/system_ext.ld", "System/applications.ld", 
                    "system_elf.ld"] + map(lambda a: a + ".ld", applications.keys()))
n.build(outputs = systemext_elf,
        rule = "link",
        inputs = map(lambda f: get_object_file(f), system_files),
        variables = {'ldflags': '-nostartfiles -Wl,-T,System/system_ext_.ld -mcpu=cortex-a9 -g3 -gdwarf-3'},
        implicit = ["System/system_ext_.ld", "System/applications.ld", 
                    "system_elf_.ld"] + map(lambda a: a + ".ld", applications.keys()))


n.build(outputs = systemextbin,
        rule = "objcopy",
        inputs = systemextelf,
        variables = {'ocflags': '-O binary'})
n.build(outputs = systemextuimg,
        rule = "mkimage",
        inputs = systemextbin)


for a in applications:
    elffile = a + ".elf"
    ldfile = a + ".ld"
    n.build(outputs = elffile,
            rule = "link",
            inputs = map(lambda f: get_object_file(f), applications[a]),
#            variables = {'ldflags': '-nostartfiles -mcpucortex-a9 -g3 -fPIC -gdwarf-3 -Wl,--unresolved-symbols=ignore-all -nostdlib' })
            variables = {'ldflags': '-nostartfiles -TApp/app.ld -mcpucortex-a9 -g3 -fPIC -gdwarf-3 -shared' },
            implicit = "App/app.ld")
                         #'libs': 'system.so'},
            #implicit = [systemso])
    n.build(outputs = ldfile,
            rule = "app_ld",
            inputs = elffile)

n.default(systemextuimg)

# for a in applications:
#     elffile = a + ".elf"
#     binfile = a + ".bin"
#     uimgfile = a + ".uimg"
#     n.build(outputs = elffile,
#             rule = "link",
#             inputs = map(lambda f: get_object_file(f), freertos_files + applications[a]),
#             implicit = 'App/plain.ld',
#             variables = {'ldflags': '-nostartfiles -Wl,-T,App/plain.ld -mcpu=cortex-a9 -g3 -gdwarf-3'})
#     n.build(outputs = binfile,
#             rule = "objcopy",
#             inputs = elffile)
#     n.build(outputs = uimgfile,
#             rule = "mkimage",
#             inputs = binfile)

            
