project('tracua_chip8', 'c',
	version : '0.1',
	default_options : ['warning_level=3', 'werror=true']
)

sdl2_dep = dependency('sdl2')

cc = meson.get_compiler('c')

subdir('src')

executable('tracua-chip8-emulator',
	emulator_src,
	dependencies : [sdl2_dep],
	install : false,
	include_directories: [
		'include'
	],
)

executable('tracua-chip8-assembler',
	assembler_src,
	install : false,
	include_directories: [
		'include'
	],
)

executable('tracua-chip8-disassembler',
	disassembler_src,
	install : false,
	include_directories: [
		'include'
	],
)