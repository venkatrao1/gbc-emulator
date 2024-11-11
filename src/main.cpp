#include <gb/ui/ui.h>
#include <gb/utils/log.h>

#include <fstream>
#include <iostream>
#include <string.h>


int main(int argc, char* argv[]) {
	if(argc < 2) {
		const char* binary_name = argv[0] ? argv[0] : "<binary>";
		std::cerr << "Usage: " << binary_name << " <gui type> ...\n";
		return 1;
	}

	try {
		auto ui = gb::ui::UI::create(argv[1], argc, argv);
		ui->main_loop();
		gb::log_info("Exiting with code 0");
		return 0;
	} catch (const std::exception& e) {
		std::cerr << "Uncaught exception: " << e.what() << '\n';
		if(errno) {
			char strerror_buf[256]{};
			strerror_s(strerror_buf, sizeof(strerror_buf), errno);
			std::cerr << "errno is \"" << strerror_buf << "\"\n";
		}
		return 2;
	}
}