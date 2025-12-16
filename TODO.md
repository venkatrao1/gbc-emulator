emulation features (in order of importance):
- audio
- randomly init mem except for i/o regs and such
- cgb mode (and sgb/gba, revisions, etc)

code/dev improvements:
- get WSL/linux build; allow building only certain UIs
- add backtrace library
- add testing library (boost prob) + test suites (mooneye, blarg, acid2)
	- CI? probably means getting a WSL/linux build working
	- test TASes (locally due to copyrighted ROMs)
- nicer imgui font?
- add debugger (+ disassembler?) - can use imgui_memory_editor as a helper
- split main into GUI + headless (headless for test running, etc)
- move things into cpp files / actually think about compile times

UX improvements:
- make an actually usable UI
- web UI??
- use open source boot ROM?
- runahead?
- screen filters? (based on real hardware, or upscaling)
- audio filters?