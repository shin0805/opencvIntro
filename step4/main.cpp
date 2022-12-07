#include <opencv2/opencv.hpp>

int main() {
  cv::Mat img;
  // /dev/video0でVideoCaptureを宣言
  cv::VideoCapture cap(0);

  while (true) {
    // カメラの画像をimgに代入
    cap.read(img);

    // imgの表示
    cv::imshow("VideoCapture", img);

    // escで終了
    unsigned char key = cv::waitKey(2);
    if (key == '\x1b') break;
  }

  return 0;
}
