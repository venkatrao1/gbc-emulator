In order of importance:
- ppu (+ gui?)
- audio
- randomly init mem except for i/o regs and such

TODO:
- add backtrace library
- add testing library (boost prob) + test suites (mooneye, blarg, acid2)
	- serial (for mooneye/blarg)
- nicer imgui font?
- add debugger - can use imgui_memory_editor as a helper
- split main into GUI + headless (headless for test running, etc)
- move things into cpp files / actually think about compile times