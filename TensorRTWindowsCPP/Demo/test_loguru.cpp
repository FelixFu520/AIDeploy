#include <chrono>
#include <thread>

#include "loguru.hpp"
//#include "loguru.cpp"
inline void sleep_ms(int ms)
{
	VLOG_F(2, "Sleeping for %d ms", ms);
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline void complex_calculation()
{
	LOG_SCOPE_F(INFO, "complex_calculation");
	LOG_F(INFO, "Starting time machine...");
	sleep_ms(200);
	LOG_F(WARNING, "The flux capacitor is not getting enough power!");
	sleep_ms(400);
	LOG_F(INFO, "Lighting strike!");
	VLOG_F(1, "Found 1.21 gigawatts...");
	sleep_ms(400);
	std::thread([]() {
		loguru::set_thread_name("the past");
		LOG_F(ERROR, "We ended up in 1985!");
	}).join();
}

int main2(int argc, char* argv[])
{
	//// ���ս���ʱ,argc=1,argv[0]Ϊexe��·��
	//argc��һ���ж��ٸ�����
	argc = 3;
	std::string aa = "TRT";
	std::string aa1 = "-v1";
	argv[1] = const_cast<char*>(aa.c_str());;
	argv[2] = const_cast<char*>(aa1.c_str());;
	// �����Ҫ����Ӳ���,argc������һ,argv�����������淽ʽ��ֵ
	//loguru::init(argc, argv);
	loguru::add_file("../everything.log", loguru::Append, loguru::Verbosity_MAX);
	LOG_F(INFO, "Hello from main.cpp!");
	LOG_F(INFO, "Hello from main.cpp!");
	LOG_F(INFO, "Hello from main.cpp!");
	LOG_F(INFO, "Hello from main.cpp!");
	//complex_calculation();
	//LOG_F(INFO, "main function about to end!");
}