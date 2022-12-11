#include <AL/alure.h>
#include <AL/alut.h>
#include <opencv2/opencv.hpp>

// ビープ音を鳴らす
void beep(double hz, double time) {
  const int SAMPLINGRATE = 44100;

  int data_size = SAMPLINGRATE * time;
  unsigned char *data = new unsigned char[data_size];

  ALuint buffer, source;
  alutInit(0, 0);
  alGenBuffers(1, &buffer);
  alGenSources(1, &source);

  for (int i = 0; i < data_size; i++) {
    data[i] = (sin(2 * M_PI * hz / SAMPLINGRATE * i) + 1) * 100;
  }

  alBufferData(buffer, AL_FORMAT_MONO8, data, data_size, SAMPLINGRATE);
  alSourcei(source, AL_BUFFER, buffer);
  alSourcePlay(source);
}

// 音階のwavを再生する
void beep(std::string note, double time) {
  std::string dir("../sound/");
  ALuint source;
  alGenSources(1, &source);
  ALuint buffer = alureCreateBufferFromFile((dir + note + ".wav").c_str());
  alSourcei(source, AL_BUFFER, buffer);
  alSourcePlay(source);
}

int main(int argc, char **argv) {
  cv::Mat frame;
  cv::Mat frame_canny;
  cv::Mat dst_img;
  cv::Mat dst_bin_img;
  std::vector<cv::Vec2f> lines;

  alureInitDevice(NULL, NULL);

  // 指定された番号のカメラに対するキャプチャオブジェクトを作成する
  cv::VideoCapture capture(0);

  // 表示用ウィンドウをの初期化
  cv::namedWindow("Score Scanner", cv::WINDOW_AUTOSIZE | cv::WINDOW_FREERATIO);

  // 周波数音階
  std::vector<double> f_notes;
  double f = 220 * pow(2., 1. / 4.);
  for (int i = 0; i < 14; ++i) {
    f_notes.push_back(f);
    if (i % 7 == 2 || i % 7 == 6) {
      f *= pow(2., 1. / 12.);
    } else {
      f *= pow(2., 1. / 6.);
    }
  }
  // 記号音階
  std::vector<std::string> treble_notes = {"C4", "D4", "E4", "F4", "G4",
                                           "A4", "B4", "C5", "D5", "E5",
                                           "F5", "G5", "A5", "B5"};

  std::vector<double> scan_x(5, 20);
  std::vector<double> scan_y(5, 0);
  std::vector<int> y_targets = {4, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0, 0, 0, 0};
  std::vector<double> width_coef = {1, 0.5, 0, 0.5, 0, 0.5, 0,
                                    0.5, 0, 0.5, 0, -0.5, -1, -1.5};
  std::vector<std::string> found_note;

  while (capture.isOpened()) {
    // カメラから画像をキャプチャする
    capture.read(frame);
    dst_img = frame.clone();

    // 二値化
    cv::cvtColor(dst_img, dst_bin_img, cv::COLOR_BGR2GRAY);
    cv::threshold(dst_bin_img, dst_bin_img, 70, 255, cv::THRESH_BINARY);

    // カメラ画像の表示
    cv::Canny(dst_bin_img, frame_canny, 50.0, 200.0);
    // 古典的Hough変換
    // 入力画像，出力，距離分解能，角度分解能，閾値，*,*
    cv::HoughLines(frame_canny, lines, 0.5, CV_PI / 360, 150, 0, 0); // 140
    std::vector<cv::Vec2f>::iterator it = lines.begin();

    // 高さで検出点をソートする
    sort(lines.begin(), lines.end(),
         [](const cv::Vec2f &alpha, const cv::Vec2f &beta) {
           return alpha[0] < beta[0];
         });

    double past_x0 = 0;
    double past_y0 = 0;
    int line_y_index = 0;

    std::vector<double> line_x(10, 0);
    std::vector<double> line_y(10, 0);
    std::vector<float> line_th(10, 0);

    for (; it != lines.end(); ++it) {
      float rho = (*it)[0], theta = (*it)[1];
      cv::Point pt1, pt2;
      double a = cos(theta), b = sin(theta);
      double x0 = a * rho, y0 = b * rho;

      // 認識点の描画
      cv::circle(dst_img, cv::Point(x0, y0), 2, cv::Scalar(255, 0, 0), -1, cv::LINE_AA);

      // 一本の線で複数の線を読まないようにする and 水平な線に近いものを読む
      if (std::hypot(x0 - past_x0, y0 - past_y0) >= 16 && abs(theta - CV_PI / 2) < 0.1) {
        // 認識した五線の描画
        pt1.x = cv::saturate_cast<int>(x0 + 1300 * (-b));
        pt1.y = cv::saturate_cast<int>(y0 + 1300 * (a));
        pt2.x = cv::saturate_cast<int>(x0 - 1300 * (-b));
        pt2.y = cv::saturate_cast<int>(y0 - 1300 * (a));
        cv::line(dst_img, pt1, pt2, cv::Scalar(0, 0, 255), 3, cv::LINE_AA);
        if (line_y_index < 10) {
          line_x[line_y_index] = x0;
          line_y[line_y_index] = y0;
          line_th[line_y_index] = theta;
          line_y_index++;
        }
      }
      past_x0 = x0;
      past_y0 = y0;
    }

    // 五線が正しく認識できていれば、楽譜を左からscan
    bool line_is_scanned;
    bool line_is_five;
    line_is_scanned = std::none_of(line_y.begin(), (line_y.begin() + 4), [](int x) { return x == 0; });
    line_is_five = abs((line_y[0] + line_y[4]) / 2 - line_y[2]) <= 20;
    if (line_is_scanned && line_is_five) {
      // scanしているy座標を決定
      for (int i = 0; i < 5; ++i) {
        if (i != 0) scan_x[i] = (line_y[i + 1] - line_y[i]);
        scan_y[i] = line_y[i] + (scan_x[0] - line_x[i]) * tan(line_th[i] - M_PI / 2);
      }
      // 五線譜の幅を概算
      double width = 0;
      for (int i = 0; i < 4; ++i) {
        width += scan_y[i + 1] - scan_y[i];
      }
      width = static_cast<int>(width / 4);
      // スキャニングバーの描画
      cv::line(dst_img, cv::Point(scan_x[0], scan_y[0] - width * 2),
               cv::Point(scan_x[0], scan_y[4] + width * 2),
               cv::Scalar(255, 204, 51), 3, cv::LINE_AA);
      // 音符の認識
      int range_x = width / 7;
      int range_y = width / 7;
      int note_index = 0;
      static double found_x = -100;
      if (abs(found_x - scan_x[0]) >= 2) {
        found_note = {};
        for (auto note : treble_notes) {
          int sum = 0;
          for (int i = -range_y; i <= range_y; i++) {
            for (int j = -range_x; j <= range_x; j++) {
              // 五線間なら
              if (note_index >= 3 && note_index <= 9 && note_index % 2 == 1) {
                int center = (scan_y[4 - (note_index - 1) / 2 + 1] + scan_y[4 - (note_index - 1) / 2]) / 2;
                sum += static_cast<int>(dst_bin_img.at<unsigned char>(cv::Point(scan_x[0] + j, center + i)));
              } else {
                sum += static_cast<int>(dst_bin_img.at<unsigned char>(cv::Point(
                    scan_x[0] + j, scan_y[y_targets[note_index]] + i + width * width_coef[note_index])));
              }
            }
          }
          if (sum == 0) {
            found_note.push_back(note);
            found_x = scan_x[0];
          }
          note_index++;
        }
        // 認識した音符の確認
        if (found_note.size() != 0) std::cout << "treble note : ";

        for_each(found_note.begin(), found_note.end(), [](const std::string &n) { std::cout << n << " "; });
        if (found_note.size() == 1) {
          for_each(found_note.begin(), found_note.end(), [&](const std::string &n) {
                     beep(n, 0.2);
                   });
        }
        if (found_note.size() != 0) std::cout << std::endl;
      }
      scan_x[0] <= 1150 ? scan_x[0] += 16 : scan_x[0] = 10;
    }

    cv::imshow("Score Scanner", dst_img);
    cv::moveWindow("Score Scanner", 1000, 350);

    // escで終了
    unsigned char key = cv::waitKey(2);
    if (key == '\x1b')
      break;
  }

  // cv::destroyWindow("Score Scanner");

  return 0;
}
