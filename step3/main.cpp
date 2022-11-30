#include <opencv2/opencv.hpp>

int main() {
  // image.pngをimgに代入
  cv::Mat img = cv::imread("../image.jpg");

  // Canny法によるエッジ検出
  cv::Mat img_canny;
  cv::Canny(img, img_canny, 500.0, 700.0);

  // img_cannyの表示
  cv::imshow("img_canny", img_canny);

  // キーが押されるまで待機
  cv::waitKey(0);

  // ハフ変換
  std::vector<cv::Vec2f> lines;
  cv::HoughLines(img_canny, lines, 0.5, CV_PI / 360, 80);

  std::vector<cv::Vec2f>::iterator it = lines.begin();
  for (; it != lines.end(); ++it) {
    float rho = (*it)[0], theta = (*it)[1];
    cv::Point pt1, pt2;
    double a = cos(theta), b = sin(theta);
    double x0 = a * rho, y0 = b * rho;
    pt1.x = cv::saturate_cast<int>(x0 + 1000 * (-b));
    pt1.y = cv::saturate_cast<int>(y0 + 1000 * (a));
    pt2.x = cv::saturate_cast<int>(x0 - 1000 * (-b));
    pt2.y = cv::saturate_cast<int>(y0 - 1000 * (a));
    cv::line(img, pt1, pt2, cv::Scalar(0, 0, 255), 3, cv::LINE_AA);
  }

  // imgの表示
  cv::imshow("img", img);

  // キーが押されるまで待機
  cv::waitKey(0);

  return 0;
}
