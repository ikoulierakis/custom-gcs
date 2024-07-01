#include <opencv2/opencv.hpp>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

std::queue<cv::Mat> frameQueue;
std::mutex mtx;
std::condition_variable condVar;

void receive() {
    cv::VideoCapture cap("udp://@127.0.0.1:1234");
    if (!cap.isOpened()) {
        std::cerr << "Error opening video stream" << std::endl;
        return;
    }

    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) {
            break;
        }
        {
            std::lock_guard<std::mutex> lock(mtx);
            frameQueue.push(frame);
        }
        condVar.notify_one();
    }
}

void display() {
    cv::namedWindow("Video", cv::WINDOW_NORMAL);
    // cv::moveWindow("Video", 40, 30);

    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        condVar.wait(lock, [] { return !frameQueue.empty(); });

        cv::Mat frame = frameQueue.front();
        frameQueue.pop();
        lock.unlock();

        if (!frame.empty()) {
            cv::resize(frame, frame, cv::Size(1080, 720));
            cv::imshow("Video", frame);
        }

        if (cv::waitKey(1) == 27) {  // press 'ESC' to quit
            break;
        }
    }
}

int main() {
    std::thread tr(receive);
    std::thread td(display);

    tr.detach();
    td.join();

    return 0;
}
