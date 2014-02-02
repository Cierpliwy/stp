env = Environment()

mymode = ARGUMENTS.get('mode','release')
if not (mymode in ['debug','release']):
    print('Error: expected debug or release in mode')
    Exit(1)
mycompiler = ARGUMENTS.get('cxx','clang++')

env.Replace(CXX = mycompiler)
env.Append(LIBS = ['ncursesw', 'pthread', 'boost_regex'])

if mymode == 'debug':
    env.Replace(CXXFLAGS = '-std=c++11 -Wall -pedantic -O0 -g -DDEBUG')
else:
    env.Replace(CXXFLAGS = '-std=c++11 -Wall -pedantic -O3')

env.Program('stp', Glob('./src/*.cpp'));
