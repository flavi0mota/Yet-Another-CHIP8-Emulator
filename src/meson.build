src = files(
	'main.c',
	'emulator.c',
	'emulated.c',
	'user_interface/sdl/interface.c',
)

executable('tracua-chip8',
	src,
	dependencies : [sdl2_dep],
	install : false,
	include_directories: [
		'../include'
	],
)