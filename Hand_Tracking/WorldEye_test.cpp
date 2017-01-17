//---------------------------------------------------------
// Abstract	 : Hand tracking program
// Library	 : OpenCV 3.1.0
// ObjectTracking.cpp
// 
// Yasuhiro Inukai before 
// Zhengqing Li 2016/08
// Updated  to github 2016/09
//---------------------------------------------------------
/*
ドラッグするとトラッキング開始。
q - プログラム終了
c - トラッキング終了
b - バックプロジェクション表示の切り替え
h - ヒストグラム表示のオンオフ
*/

#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>

#define		SEGMENT				150		//	cvSnakeImageで用いる制御点の数
#define		WINDOW_WIDTH		17		//	cvSnakeImageで最小値を探索する近傍領域の幅
#define		WINDOW_HEIGHT		17		//	cvSnakeImageで最小値を探索する近傍領域の高さ

cv::Mat	resultImage;			//	処理結果表示用cv::Mat
cv::Mat	hsvImage;               //	HSV表色系用cv::Mat
cv::Mat	hueImage;               //	HSV表色系のHチャンネル用cv::Mat
cv::Mat	maskImage;              //	マスク画像用cv::Mat
cv::Mat	backprojectImage;       //	バックプロジェクション画像用cv::Mat
cv::Mat	histImage;              //	ヒストグラム描画用cv::Mat
cv::Mat	grayImage;              //	グレースケール画像用cv::Mat

cv::Mat hist;                   //	ヒストグラム処理用cv::Mat


//	CamShiftトラッキング用変数
cv::Point			origin;
cv::Rect			selection;
cv::Rect			trackWindow;
cv::RotatedRect     trackRegion;


int
main(int argc, char *argv[])
{
	char		windowNameHistogram[] = "Histogram";			//	ヒストグラムを表示するウインドウ名
	char		windowNameObjectTracking[] = "ObjectTracking";	//	物体追跡結果を表示するウインドウ名
	char		windowObjectT[] = "ObjectT";

	cv::Mat frameImage;         //	キャプチャ画像用cv::Mat
								//cv::Mat frameImage1;


								//cv::VideoCapture capWebcam(0);		// declare a VideoCapture object and associate to webcam, 0 => use 1st webcam
								//cv::VideoCapture capWebcam1(1);

	cv::VideoCapture capWebcam("http://172.16.0.254:9176");
	//This Http address allows you to capture a stream from Kodak SP360 through WIFI
	capWebcam.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
	capWebcam.set(CV_CAP_PROP_FRAME_HEIGHT, 1280);

	/*capWebcam.set(CV_CAP_PROP_FRAME_WIDTH, 1440);
	capWebcam.set(CV_CAP_PROP_FRAME_HEIGHT, 1440);
	*///
	//capWebcam1.set(CV_CAP_PROP_FRAME_WIDTH, 1440);
	//capWebcam1.set(CV_CAP_PROP_FRAME_HEIGHT, 1440);

	//  int	i;
	//	int	j = 0;
	int		key;	//	キー入力結果を格納する変数


								//	カメラを初期化する

	if (!capWebcam.isOpened()) {
		//	カメラが見つからなかった場合
		printf("カメラが見つかりません\n");
		return -1;
	}
	//	ウインドウを生成する
	cv::namedWindow(windowNameObjectTracking, CV_WINDOW_AUTOSIZE);
	//cv::namedWindow(windowObjectT, CV_WINDOW_AUTOSIZE);

	while (1) {
		//	キャプチャ画像の取得に失敗したら処理を中断
		capWebcam >> frameImage;
		//capWebcam1 >> frameImage1;

		if (frameImage.empty()) {
			break;
		}

		//	画像を表示する

		cv::Mat	rawImage = cv::Mat(600, 800, CV_8UC3);
		cv::resize(frameImage, rawImage, rawImage.size());

		cv::Mat	resultImage = cv::Mat(600, 800, CV_8UC3);

		for (int i = 0; i < 800; i++) {
			rawImage.col(i).copyTo(resultImage.col(799-i));
		}

		cv::imshow(windowNameObjectTracking, resultImage);
		//cv::imshow(windowObjectT, frameImage1);


		//	キー入力を待ち、押されたキーによって処理を分岐させる
		key = cv::waitKey(10);
	
	}

	return 0;
}
