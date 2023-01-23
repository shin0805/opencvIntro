#include "opencv2/xfeatures2d.hpp"
#include <iostream>
#include <opencv2/opencv.hpp>

int main() {
  // source画像の準備、この画像を変換する
  cv::Mat img_source = cv::imread("../input/source.jpg");
  cv::resize(img_source, img_source, cv::Size(512, 512));
  cv::Mat img_gray_source;
  cv::cvtColor(img_source, img_gray_source, cv::COLOR_RGB2GRAY);
  cv::normalize(img_gray_source, img_gray_source, 0, 255, cv::NORM_MINMAX);

  // target画像の準備、この画像に合うように変換行列を計算する
  cv::Mat img_target = cv::imread("../input/target.jpg");
  cv::resize(img_target, img_target, cv::Size(512, 512));
  cv::Mat img_gray_target;
  cv::cvtColor(img_target, img_gray_target, cv::COLOR_RGB2GRAY);
  cv::normalize(img_gray_target, img_gray_target, 0, 255, cv::NORM_MINMAX);

  // SIFTによるキーポイントの抽出
  std::vector<cv::KeyPoint> keypoints_source, keypoints_target;
  cv::Ptr<cv::Feature2D> f2d = cv::xfeatures2d::SiftFeatureDetector::create();

  // キーポイントを見つける
  f2d->detect(img_gray_source, keypoints_source);
  f2d->detect(img_gray_target, keypoints_target);

  std::cout << "source keypoint_num :" << keypoints_source.size() << std::endl;
  std::cout << "target keypoint_num :" << keypoints_target.size() << std::endl;

  // キーポイントのDescriptor（特徴ベクトルを計算する）
  cv::Mat descriptors_source, descriptors_target;
  f2d->compute(img_gray_source, keypoints_source, descriptors_source);
  f2d->compute(img_gray_target, keypoints_target, descriptors_target);

  // BFMatcherを使ってDescriptorのマッチングを行う
  cv::BFMatcher matcher;

  // k最近防法を用いたマッチング
  std::vector<std::vector<cv::DMatch>> knn_matches;
  matcher.knnMatch(descriptors_source, descriptors_target, knn_matches, 20);

  // Loweのratio testでマッチングをフィルタリングする
  const float ratio_thresh = .75f;
  std::vector<cv::DMatch> matches;
  for (size_t i = 0; i < knn_matches.size(); i++) {
    if (knn_matches[i][0].distance <
        ratio_thresh * knn_matches[i][1].distance) {
      matches.push_back(knn_matches[i][0]);
    }
  }

  // フィルタリングされたマッチングのキーポイントを求める
  std::vector<cv::Point2f> obj;
  std::vector<cv::Point2f> scene;
  for (size_t i = 0; i < matches.size(); i++) {
    obj.push_back(keypoints_source[matches[i].queryIdx].pt);
    scene.push_back(keypoints_target[matches[i].trainIdx].pt);
  }

  // ホモグラフィ変換行列の推定
  cv::Mat masks;
  cv::Mat H = cv::findHomography(obj, scene, masks, cv::RANSAC, 3);

  // 算出したホモグラフィ変換行列を用いてperspective変換
  cv::Mat img_transformed;
  cv::warpPerspective(img_source, img_transformed, H, img_transformed.size());

  // 結果を保存
  cv::imwrite("../output/img_transformed.jpg", img_transformed);

  // マッチングの結果を描画＆保存
  cv::Mat output;
  cv::drawMatches(img_source, keypoints_source, img_target, keypoints_target,
                  matches, output);
  cv::imwrite("../output/matching_result.jpg", output);
}