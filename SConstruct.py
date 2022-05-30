#!python
import os
import sys
import subprocess

opts = Variables([], ARGUMENTS)

# Gets the standard flags CC, CCX, etc.
env = DefaultEnvironment()

# Try to detect the host platform automatically.
# This is used if no `platform` argument is passed
if sys.platform.startswith("linux"):
    host_platform = "linux"
elif sys.platform == "darwin":
    host_platform = "osx"
elif sys.platform == "win32" or sys.platform == "msys":
    host_platform = "windows"
else:
    raise ValueError("Could not detect platform automatically, please specify with platform=<platform>")

# Define our options
opts.Add(EnumVariable('target', "Compilation target", 'debug', ['d', 'debug', 'r', 'release']))
opts.Add(EnumVariable('platform', "Compilation platform", host_platform, ['', 'windows', 'x11', 'linux', 'osx']))
opts.Add(BoolVariable('use_llvm', "Use the LLVM / Clang compiler", 'no'))
opts.Add(BoolVariable('use_mingw', "Use the MINGW compiler", 'no'))
opts.Add(PathVariable('target_path', 'The path where the lib is installed.', 'bin/'))
opts.Add(EnumVariable('bits', 'Register width of target platform.', '64', ['32', '64']))

# Local dependency paths, adapt them to your setup
godot_headers_path = "godot-cpp/godot-headers/"
cpp_bindings_path = "godot-cpp/"
cpp_library = "libgodot-cpp"
gumbo_path = "sigil-gumbo/"
gumbo_library = "libgumbo"

# Updates the environment with the option variables.
opts.Update(env)

def get_cmake_generator(env) -> str:
    if env['platform'] == 'windows':
        if env['use_mingw'] or env['use_llvm']:
            return '"MinGW Makefiles"'
        else:
            # todo: Use NMake Makefiles or Visual Studio project
            raise Exception("On Windows only MinGW Makefile is supported currently")
    else:
        return '"MinGW Makefiles"'

def get_cmake_build_type(env) -> str:
    if env['target'] in ('debug', 'd'):
        return 'Debug'
    else:
        return 'Release'

def get_cmake_flags(env) -> str:
    if env['use_mingw'] or env['use_llvm']:
        if env['bits'] == '32':
            return f'-DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32 -DCMAKE_SHARED_LINKER_FLAGS=-m32'
        else:
            return f'-DCMAKE_C_FLAGS=-m64 -DCMAKE_CXX_FLAGS=-m64 -DCMAKE_SHARED_LINKER_FLAGS=-m64'
    else:
        # todo:
        return ""

def get_cmake_compilers(env) -> str:
    # todo: Use env['CC'] and env['CXX']?
    if env['use_mingw']:
        return '-DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++'
    elif env['use_llvm']:
        return '-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++'
    else:
        # todo: Eh, not ideal, but CMake could probably assume it based on host
        return ""

def run_shell(cmd: str) -> None:
    print(os.getcwd())
    print(cmd)
    popen = subprocess.Popen(cmd, stdout=subprocess.PIPE, universal_newlines=True)
    for line in iter(popen.stdout.readline, ""):
        print(line, end="")
    popen.stdout.close()
    if (return_code := popen.wait()) != 0:
        raise subprocess.CalledProcessError(return_code, cmd)

def bool_to_option(val: bool) -> str:
    return 'yes' if val else 'no'

# Process some arguments
if env['use_llvm']:
    env['CC'] = 'clang'
    env['CXX'] = 'clang++'

if env['use_mingw']:
    env['CC'] = 'gcc'
    env['CXX'] = 'g++'

if env['platform'] == '':
    print("No valid target platform selected.")
    quit();

run_shell('mkdir -p sigil-gumbo/build')
run_shell(f'cmake -S sigil-gumbo -B sigil-gumbo/build -G {get_cmake_generator(env)} -DGUMBO_STATIC_LIB=ON -DCMAKE_BUILD_TYPE={get_cmake_build_type(env)} {get_cmake_compilers(env)} {get_cmake_flags(env)}')
run_shell(f'cmake --build sigil-gumbo/build')

# todo: godot-cpp doesn't handle llvm properly under windows
run_shell(f"scons --directory \"./godot-cpp\" target={env['target']} platform={env['platform']} use_mingw={bool_to_option(env['use_mingw'])} use_llvm={bool_to_option(env['use_llvm'])} bits={env['bits']}")

# For the reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# Check our platform specifics
if env['platform'] == "osx":
    env['target_path'] += 'osx/'
    cpp_library += '.osx'
    env.Append(CCFLAGS=['-arch', 'x86_64'])
    env.Append(CXXFLAGS=['-std=c++17'])
    env.Append(LINKFLAGS=['-arch', 'x86_64'])
    if env['target'] in ('debug', 'd'):
        env.Append(CCFLAGS=['-g', '-O2'])
    else:
        env.Append(CCFLAGS=['-g', '-O3'])

elif env['platform'] in ('x11', 'linux'):
    env['target_path'] += 'x11/'
    cpp_library += '.linux'
    env.Append(CCFLAGS=['-fPIC'])
    env.Append(CXXFLAGS=['-std=c++17'])
    if env['target'] in ('debug', 'd'):
        env.Append(CCFLAGS=['-g3', '-Og'])
    else:
        env.Append(CCFLAGS=['-g', '-O3'])

elif env['platform'] == "windows":
    if env['bits'] == '64':
        env['target_path'] += 'win64/'
    else:
        env['target_path'] += 'win32/'
    cpp_library += '.windows'

    if not env['use_llvm'] and not env['use_mingw']:
        # This makes sure to keep the session environment variables on windows,
        # that way you can run scons in a vs 2017 prompt and it will find all the required tools
        env.Append(ENV=os.environ)

        env.Append(CPPDEFINES=['WIN32', '_WIN32', '_WINDOWS', '_CRT_SECURE_NO_WARNINGS'])
        env.Append(CCFLAGS=['-W3', '-GR'])
        env.Append(CXXFLAGS='/std:c++17')
        if env['target'] in ('debug', 'd'):
            env.Append(CPPDEFINES=['_DEBUG'])
            env.Append(CCFLAGS=['-EHsc', '-MDd', '-ZI'])
            env.Append(LINKFLAGS=['-DEBUG'])
        else:
            env.Append(CPPDEFINES=['NDEBUG'])
            env.Append(CCFLAGS=['-O2', '-EHsc', '-MD'])
    else:
        env.Append(LINKFLAGS=['--static', '-static-libgcc', '-static-libstdc++']) # todo: Should we?
        env.Append(CCFLAGS=['-Wall', '-Wextra'])

        if env['target'] in ('debug', 'd'):
            env.Append(CPPDEFINES=['_DEBUG'])
            env.Append(CCFLAGS=['-O0', '-g3'])
        else:
            env.Append(CPPDEFINES=['NDEBUG'])
            env.Append(CCFLAGS=['-flto', '-O3', '-g0'])


if env['target'] in ('debug', 'd'):
    cpp_library += '.debug'
else:
    cpp_library += '.release'

cpp_library += '.' + str(env['bits'])

# make sure our binding library is properly includes
env.Append(CPPPATH=['.', godot_headers_path, cpp_bindings_path + 'include/', cpp_bindings_path + 'include/core/', cpp_bindings_path + 'include/gen/', gumbo_path + 'src'])
env.Append(LIBPATH=[cpp_bindings_path + 'bin/', gumbo_path + 'build/.libs/'])
env.Append(LIBS=[cpp_library, gumbo_library])

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=['src/'])
sources = Glob('src/*.cpp')

library = env.SharedLibrary(target=env['target_path'] + 'gd-gumbo' , source=sources)

Default(library)

# Generates help for the -h scons option.
Help(opts.GenerateHelpText(env))
