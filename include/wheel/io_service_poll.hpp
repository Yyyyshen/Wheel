#ifndef io_service_poll_h__
#define io_service_poll_h__

#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <boost/asio.hpp>

/***********************在 event handler 中允许执行阻塞的操作(例如数据库查询操作)*****************************************/
namespace wheel {
	class io_service_poll {
	public:
		static io_service_poll& get_instance() {
			//c++11保证唯一性
			static io_service_poll instance;
			return instance;
		}

		~io_service_poll() {
			stop();
		}

		std::shared_ptr<boost::asio::io_service> get_main_io_service()const{
			auto ptr = io_services_[0];
			return std::move(ptr);
		}

		std::shared_ptr<boost::asio::io_service> get_io_service() {
			auto io_service = io_services_[next_io_service_];
			++next_io_service_;
			if (next_io_service_ == io_services_.size()) {
				next_io_service_ = 1;
			}
				
			return std::move(io_service);
		}

		void run(){
			std::vector<std::shared_ptr<std::thread> > threads;
			for (std::size_t i = 0; i < io_services_.size(); ++i) {
				threads.emplace_back(std::make_shared<std::thread>(
					[](io_service_ptr svr) {
						boost::system::error_code ec;
						svr->run(ec);
					}, io_services_[i]));
			}

			size_t count = threads.size();

			for (std::size_t i = 0; i < count; ++i) {
				threads[i]->join();
			}	
		}

		io_service_poll(const io_service_poll&) = delete;
		io_service_poll& operator=(const io_service_poll&) = delete;
		io_service_poll(const io_service_poll&&) = delete;
		io_service_poll& operator=(const io_service_poll&&) = delete;
	private:
		io_service_poll() {
			try
			{
				size_t count= std::thread::hardware_concurrency();
				for (size_t index =0;index< count;++index){
					auto ios = std::make_shared<boost::asio::io_service>();
					auto work = traits::make_unique<boost::asio::io_service::work>(*ios);
					works_.emplace_back(std::move(work));
					io_services_.emplace_back(std::move(ios));
				}

			}catch (const std::exception & ex){
				std::cout << ex.what() << std::endl;
				exit(0);
			}
		}
	private:
		void stop(){
			works_.clear();

			size_t count = io_services_.size();
			for (size_t index =0;index< count;++index){
				io_services_[index]->stop();
			}

			io_services_.clear();
		}
	private:
		using io_service_ptr = std::shared_ptr<boost::asio::io_service>;
		using work_ptr = std::unique_ptr<boost::asio::io_service::work>;

		size_t next_io_service_ = 1;
		std::vector<io_service_ptr>io_services_;
		std::vector<work_ptr>works_;
	};
}
#endif // io_service_poll_h__
