#include <WinSock2.h>
#include <Windows.h>
#include "cuckoo.h"
#include "vmware.h"
#include "vbox.h"
#include <conio.h>
#include <iostream>
#include "ve_detection.h"
#include <map>
#include "report.h"


#if defined(_WIN32) && defined(_WIN64)
#error "Only Win32 is supported"
#endif // _WIN32


using SandboxEvasion::VEDetection;
using SandboxEvasion::Cuckoo;
using SandboxEvasion::VMWare;
using SandboxEvasion::VBOX;
using std::iostream;

typedef VEDetection* (*fact_meth)(const json_tiny &);

static std::map<std::string, fact_meth> k_fm = {
	{ "--cuckoo",	Cuckoo::create_instance },
	{ "--vmware",	VMWare::create_instance },
	{ "--vbox",		VBOX::create_instance }
	// FIXME: add other
};

static args_t k_args = {
	{ "--cuckoo",	NULL },
	{ "--generic",	NULL },
	{ "--vmware",	NULL },
	{ "--vbox",		NULL }
};

void test() {
	// Create a root
	/*
	pt::ptree root;
	pt::ptree root2;

	try {
	// Load the json file in this ptree
	pt::read_json("cuckoo.conf", root);
	}
	catch (const std::exception &e) {
	return;
	}

	BOOST_FOREACH(pt::ptree::value_type &p, root) {
		std::cout << p.first << std::endl;
	}

	// Read values
	int height = root.get<int>("height", 0);

	// emulate animals
	// A vector to allow storing our animals
	std::vector< std::pair<std::string, std::string> > animals;

	// Iterator over all animals
	for (pt::ptree::value_type &animal : root.get_child("animals"))
	{
	// Animal is a std::pair of a string and a child

	// Get the label of the node
	std::string name = animal.first;
	// Get the content of the node
	//std::string color = animal.second.data();
	//animals.push_back(std::make_pair(name, color));

	}

	root2 = root.get_child("animals");

	std::string r = root2.get<std::string>("rabbit.color");
	int size = root2.get<int>("rabbit.size");

	std::cout << size << std::endl;
	std::cout << r << std::endl;

	// You can also go through nested nodes
	std::string msg = root.get<std::string>("some.complex.path", "asdfasdf");
	*/
}

void test_report() {
	Report report;

	if (!report.load())
		return;

	report.add_entry({"Blabla", "XXX", "1", "YYY"});
	report.add_entry({ "Blabla", "XXX", "0", "YYY" });
	report.add_entry({ "Blabla", "XXX", "2", "YYY" });
	report.add_entry({ "Blabla", "XXX", "1", "YYY" });

	if (!report.flush("Cuckoo"))
		return;

	report.add_entry({ "asdf", "XXX", "3", "YYY" });
	report.add_entry({ "Blasdgabla", "XXX", "0", "YYY" });
	report.add_entry({ "asdg", "XXX", "2", "YYY" });
	report.add_entry({ "Basdglabla", "XXX", "1", "YYY" });

	if (!report.flush("VBOX"))
		return;

	report.dump("report_exec.html");
}


void perform_action(const char *action) {
	Cuckoo cuckoo = Cuckoo(json_tiny());

	/*
	 * Cuckoo actions
	 */
	if (!strncmp(action, "--pid", 5)) {
		cuckoo.IsPidReusedNotTracked(SandboxEvasion::ProcessWorkingMode::SLAVE);
	}
	else if (!strncmp(action, "--evt", 5)) {
		// just sleep for some time and then exit process
		Sleep(1000);
		ExitProcess(0);
	}
	else if (!strncmp(action, "--exc", 5)) {
		cuckoo.CheckExceptionsNumber(SandboxEvasion::ProcessWorkingMode::SLAVE);
	}
	else if (!strncmp(action, "--wmi", 5)) {
		cuckoo.IsWMINotTracked(SandboxEvasion::ProcessWorkingMode::SLAVE);
	}
	else if (!strncmp(action, "--tsh", 5)) {
		/*
		wsprintfW(fname, L"E:\\tmp\\tsh_%d", GetCurrentProcessId());
		CreateFileW(fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		*/
		cuckoo.IsTaskSchedNotTracked(SandboxEvasion::ProcessWorkingMode::SLAVE);
	}
}


void banner() {
	std::string module_name("");

	log_message(LogMessageLevel::NO, module_name, std::string(30, '*'), BLUE);
	log_message(LogMessageLevel::NO, module_name, "********SandboxEvasion********", BLUE);
	log_message(LogMessageLevel::NO, module_name, std::string(30, '*'), BLUE);
}


int main(int argc, char **argv, char **env) {
	int arg_no;
	args_t args;
	const char verbose_mode[] = "--verbose";
	const char action_mode[] = "--action";
	std::string module_name("MAIN");
	std::list<VEDetection*> detects;
	std::list<json_tiny *> jsons;
	json_tiny *pj;
	bool action = false;
	char *chosen_action = NULL;
	Report report;
	Report *pReport;
	char report_file[MAX_PATH] = {};
	SYSTEMTIME st;

	// test_report();

	// test();

	// TODO: RAR SFX problem
	// TODO: do we need to disable FsRedirection

	if (argc < 2)
		return 0;

	TORS_ROUTINE ctors_r[] = { ctors_wsa, ctors_check_wow64, ctors_get_os_ver };
	TORS_ROUTINE dtors_r[] = { dtors_wsa };

	if (!ctors(ctors_r, _countof(ctors_r))) {
		enable_verbose_mode();
		log_message(LogMessageLevel::ERR, module_name, "Unable to initialize constructors. Exiting...");
		return 1;
	}

	// parse incoming arguments
	for (auto &a : k_args) {
		arg_no = 1;
		while (arg_no < argc) {
			if (!strncmp(a.first, argv[arg_no], strlen(a.first)) && arg_no + 1 < argc) {
				a.second = argv[arg_no + 1];
				++arg_no;
			}
			else if (!strncmp(verbose_mode, argv[arg_no], strlen(verbose_mode))) {
				enable_verbose_mode();
			}
			else if (!strncmp(action_mode, argv[arg_no], strlen(action_mode)) && arg_no + 1 < argc) {
				action = true;
				chosen_action = argv[arg_no + 1];
				++arg_no;
			}
			++arg_no;
		}
	}

	if (action && chosen_action) {
		perform_action(chosen_action);
		return 0;
	}

	banner();

	pReport = report.load() ? &report : NULL;

	// printf debug info
	log_message(LogMessageLevel::INFO, module_name, std::string("Initialize virtual environment detection modules..."));

	for (auto &a : k_args) {
		if (a.second) {
			pj = json_tiny::load(a.second);
			if (pj) {
				detects.push_back(k_fm[a.first](*pj));
				jsons.push_back(pj);
			}
		}
	}

	// printf info
	for (auto &d : detects) {
		log_message(LogMessageLevel::INFO, d->GetModuleName(), std::string("Starting checks..."));
		d->AddReportModule(pReport);
		d->CheckAll();
		log_message(LogMessageLevel::INFO, d->GetModuleName(), std::string("Checks finished\n") + std::string(60, '*') + std::string("\n"));
	}

	if (pReport) {
		GetSystemTime(&st);
		_snprintf_s(report_file, _countof(report_file), "SandboxEvasion_%.02u%.02u%.02u_%.02u%.02u%.02u.html", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

		if (report.dump(report_file))
			log_message(LogMessageLevel::INFO, module_name, std::string("Report was successfully saved to file: ") + std::string(report_file));
	}

	// free all data
	for (auto &d : detects)
		delete[]d;

	for (auto &j : jsons)
		delete[]j;

	// FIXME: should be removed
	log_message(LogMessageLevel::INFO, module_name, std::string("Press key to exit..."));
	_getch();

	dtors(dtors_r, _countof(dtors_r));

	return 0;
}


// Cuckoo Sandbox testing
#if 0
int main(int argc, char **argv, char **env) {
	Cuckoo cd;
	TORS_ROUTINE ctors_r[] = { ctors_wsa };
	TORS_ROUTINE dtors_r[] = { dtors_wsa };

	if (!ctors(ctors_r, _countof(ctors_r))) {
		fprintf(stdout, "[+] Unable to init constructors. Exiting...\n");
		return 1;
	}


	// FIXME: delete
	/*
	wchar_t fname[64] = {};
	wsprintfW(fname, L"E:\\tmp\\s_%d", GetCurrentProcessId());
	HANDLE hFile;
	DWORD written;

	if ((hFile = CreateFileW(fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
	fprintf(stdout, "file error: %x\n", GetLastError());
	}

	WriteFile(hFile, argv[0], strlen(argv[0]), &written, NULL);
	if (argc > 1) {
	WriteFile(hFile, "\r\n", 2, &written, NULL);
	WriteFile(hFile, argv[1], strlen(argv[1]), &written, NULL);
	}
	CloseHandle(hFile);
	*/

	// checking mode
	if (argc < 2) {
		fprintf(stdout, "[+] Unbalanced stack detected: %u\n", cd.CheckUnbalancedStack());
		fprintf(stdout, "[+] INFINITE delay skipping detected: %u\n", cd.CheckInfiniteSleep());
		fprintf(stdout, "[+] Delays accumulation detected: %u\n", cd.CheckDelaysAccumulation());
		fprintf(stdout, "[+] Functions hooking detected: %u\n", cd.CheckFunctionHooks());
		fprintf(stdout, "[+] Agent artifacts detected: %u\n", cd.CheckAgentArtifacts());
		fprintf(stdout, "[+] Cuckoomon configuration detected: %u\n", cd.IsConfigurationPresent());
		fprintf(stdout, "[+] Whitelisted process escape detected: %u\n", cd.IsWhitelistedNotTracked());
		fprintf(stdout, "[+] Event name detected: %u\n", cd.CheckEventName());
		fprintf(stdout, "[+] Exceptions escape detected: %u\n", cd.CheckExceptionsNumber(SandboxEvasion::ProcessWorkingMode::MASTER));
		fprintf(stdout, "[+] WMI escape detected: %u\n", cd.IsWMINotTracked(SandboxEvasion::ProcessWorkingMode::MASTER));
		fprintf(stdout, "[+] Task Scheduler escape detected: %u\n", cd.IsTaskSchedNotTracked(SandboxEvasion::ProcessWorkingMode::MASTER));
		fprintf(stdout, "[+] Pid reusage escape detected: %u\n", cd.IsPidReusedNotTracked(SandboxEvasion::ProcessWorkingMode::MASTER));
		fprintf(stdout, "[+] Agent detected: %u\n", cd.IsAgentPresent());
		fflush(stdout);

		_getch();
	}
	else {
		// FIXME: add modes parsing
		if (argc < 3) {
			if (!strncmp(argv[1], "--pid", 5)) {
				cd.IsPidReusedNotTracked(SandboxEvasion::ProcessWorkingMode::SLAVE);
			}
			else if (!strncmp(argv[1], "--evt", 5)) {
				// just sleep for some time and then exit process
				Sleep(1000);
				ExitProcess(0);
			}
			else if (!strncmp(argv[1], "--exc", 5)) {
				cd.CheckExceptionsNumber(SandboxEvasion::ProcessWorkingMode::SLAVE);
			}
			else if (!strncmp(argv[1], "--wmi", 5)) {
				cd.IsWMINotTracked(SandboxEvasion::ProcessWorkingMode::SLAVE);
			}
			else if (!strncmp(argv[1], "--tsh", 5)) {
				/*
				wsprintfW(fname, L"E:\\tmp\\tsh_%d", GetCurrentProcessId());
				CreateFileW(fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				*/
				cd.IsTaskSchedNotTracked(SandboxEvasion::ProcessWorkingMode::SLAVE);
			}
		}

	}

	dtors(dtors_r, _countof(dtors_r));

	return 0;
}
#endif