#include <opencv2/opencv.hpp>

int main()
{
    // image.pngをimgに代入
    cv::Mat img = cv::imread("../image.jpg");

    // imgの表示
    cv::imshow("img", img);

    // キーが押されるまで待機
    cv::waitKey(0);

    return 0;
}
