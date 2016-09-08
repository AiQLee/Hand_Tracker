//---------------------------------------------------------
// Abstract	 : Hand tracking program
// Library	 : OpenCV 3.1.0
// ObjectTracking.cpp
// 
// Yasuhiro Inukai before 
// Zhengqing Li 2016/08
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
#define		HISTIMAGE_WIDTH		320		//	ヒストグラム画像の幅
#define		HISTIMAGE_HEIGHT	200		//	ヒストグラム画像の高さ
#define		H_DIMENSION		16		//	ヒストグラムの次元数
#define		H_RANGE_MIN		0
#define		H_RANGE_MAX		180
#define		V_MIN	10		//	明度の最小値
#define		V_MAX	256		//	明度の最大値
#define		S_MIN	30		//	彩度の最小値
#define		S_MAX	256		//	彩度の最小値
#define		HIDDEN_BACKPROJECTION	0	//	バックプロジェクション画像を表示させないフラグ値
#define		SHOW_BACKPROJECTION		1	//	バックプロジェクション画像を表示させるフラグ値
#define		SELECT_OFF				0	//	初期追跡領域が設定されていないときのフラグ値
#define		SELECT_ON				1	//	初期追跡領域が設定されているときのフラグ値
#define		TRACKING_STOP			0	//	トラッキングを止めるフラグ値
#define		TRACKING_START			-1	//	トラッキングを開始するフラグ値
#define		TRACKING_NOW			1	//	トラッキング中を示すフラグ値
#define		HIDDEN_HISTOGRAM		0	//	ヒストグラムを表示させないフラグ値
#define		SHOW_HISTOGRAM			1	//	ヒストグラムを表示させるフラグ値
#define		ITERATION_SNAKE			10	//	cvSnakeImageの反復回数


cv::Mat	resultImage;			//	処理結果表示用cv::Mat
cv::Mat	hsvImage;               //	HSV表色系用cv::Mat
cv::Mat	hueImage;               //	HSV表色系のHチャンネル用cv::Mat
cv::Mat	maskImage;              //	マスク画像用cv::Mat
cv::Mat	backprojectImage;       //	バックプロジェクション画像用cv::Mat
cv::Mat	histImage;              //	ヒストグラム描画用cv::Mat
cv::Mat	grayImage;              //	グレースケール画像用cv::Mat

cv::Mat hist;                   //	ヒストグラム処理用cv::Mat

								//	処理モード選択用フラグ
int	backprojectMode = HIDDEN_BACKPROJECTION;
int	selectObject = SELECT_OFF;
int	trackObject = TRACKING_STOP;
int showHist = SHOW_HISTOGRAM;

//	CamShiftトラッキング用変数
cv::Point			origin;
cv::Rect			selection;
cv::Rect			trackWindow;
cv::RotatedRect     trackRegion;

//	ヒストグラム用変数
int		hdims = H_DIMENSION;		//	ヒストグラムの次元数
float	hRangesArray[] = { H_RANGE_MIN, H_RANGE_MAX };	//ヒストグラムのレンジ
float	*hRanges = hRangesArray;
int		vmin = V_MIN;
int		vmax = V_MAX;
int     hChannels[] = { 0 };
const float* histRanges[] = { hRanges };



//
//	マウスドラッグによって初期追跡領域を指定する
//
//	引数:
//		event	: マウス左ボタンの状態
//		x		: マウスが現在ポイントしているx座標
//		y		: マウスが現在ポイントしているy座標
//		flags	: 本プログラムでは未使用
//		param	: 本プログラムでは未使用
//
void on_mouse(int event, int x, int y, int flags, void* param) {
	//	画像が取得されていなければ、処理を行わない
	if (resultImage.empty()) {
		return;
	}

	//	マウスの左ボタンが押されていれば以下の処理を行う
	if (selectObject == SELECT_ON) {
		selection.x = MIN(x, origin.x);
		selection.y = MIN(y, origin.y);
		selection.width = selection.x + CV_IABS(x - origin.x);
		selection.height = selection.y + CV_IABS(y - origin.y);

		selection.x = MAX(selection.x, 0);
		selection.y = MAX(selection.y, 0);
		selection.width = MIN(selection.width, resultImage.size().width);
		selection.height = MIN(selection.height, resultImage.size().height);
		selection.width = selection.width - selection.x;
		selection.height = selection.height - selection.y;
	}
	//	マウスの左ボタンの状態によって処理を分岐
	switch (event) {
	case CV_EVENT_LBUTTONDOWN:
		//	マウスの左ボタンが押されたのであれば、
		//	原点および選択された領域を設定
		origin = cv::Point(x, y);
		selection = cv::Rect(x, y, 0, 0);
		selectObject = SELECT_ON;
		break;
	case CV_EVENT_LBUTTONUP:
		//	マウスの左ボタンが離されたとき、widthとheightがどちらも正であれば、
		//	trackObjectフラグをTRACKING_STARTにする
		selectObject = SELECT_OFF;
		if (selection.width > 0 && selection.height > 0) {
			trackObject = TRACKING_START;
		}
		break;
	}
}



//
//	入力された1つの色相値をRGBに変換する
//
//	引数:
//		hue		: HSV表色系における色相値H
//	戻り値：
//		cv::Scalar: RGBの色情報がBGRの順で格納されたコンテナ
//
cv::Scalar hsv2rgb(float hue) {
	cv::Mat rgbValue, hsvValue;
	rgbValue = cv::Mat(1, 1, CV_8UC3);
	hsvValue = cv::Mat(1, 1, CV_8UC3);

	// 色相値H, 彩度値S, 明度値V
	hsvValue.at<cv::Vec3b>(0) = cv::Vec3b(cv::saturate_cast<uchar>(hue), 255, 255);

	//	HSV表色系をRGB表色系に変換する
	cv::cvtColor(hsvValue, rgbValue, CV_HSV2BGR);
	return cv::Scalar((unsigned char)rgbValue.at<cv::Vec3b>(0)[0],
		(unsigned char)rgbValue.at<cv::Vec3b>(0)[1],
		(unsigned char)rgbValue.at<cv::Vec3b>(0)[2], 0);
}



//
//	マウス選択された初期追跡領域におけるHSVのH値でヒストグラムを作成し、ヒストグラムの描画までを行う
//
//	引数:
//		hist		: mainで宣言されたヒストグラム用構造体
//		hsvImage	: 入力画像がHSV表色系に変換された後のIplImage
//		maskImage	: マスク画像用IplImage
//		selection	: マウスで選択された矩形領域
//
cv::Mat CalculateHist(cv::Mat hist, cv::Mat hsvImage, cv::Mat maskImage, cv::Rect selection) {
	int		i;
	int		binW;	//	ヒストグラムの各ビンの、画像上での幅
	int		val;	//	ヒストグラムの頻度
	double	maxVal;	//	ヒストグラムの最大頻度
	std::vector<cv::Mat> mv;    // for split image

								//	hsv画像の各画素が値の範囲内に入っているかチェックし、
								//	マスク画像maskImageを作成する
	cv::inRange(hsvImage,
		cv::Scalar(H_RANGE_MIN, S_MIN, MIN(V_MIN, V_MAX), 0),
		cv::Scalar(H_RANGE_MAX, S_MAX, MAX(V_MIN, V_MAX), 0),
		maskImage);

	//	hsvImageのうち、とくに必要なHチャンネルをhueImageとして分離する
	cv::split(hsvImage, mv);
	hueImage = mv[0];   //hue

						//	trackObjectがTRACKING_START状態なら、以下の処理を行う
	if (trackObject == TRACKING_START) {
		//	追跡領域のヒストグラム計算とhistImageへの描画
		maxVal = 0.0;

		cv::Mat roi(hueImage, selection);
		cv::Mat maskroi(maskImage, selection);

		//	ヒストグラムを計算
		cv::calcHist(&roi, 1, hChannels, maskroi, hist, 1, &hdims, histRanges);
		//	ヒストグラムの縦軸（頻度）を0-255のダイナミックレンジに正規化
		cv::normalize(hist, hist, 0, 255, CV_MINMAX);

		trackWindow = selection;
		//	trackObjectをTRACKING_NOWにする
		trackObject = TRACKING_NOW;

		//	ヒストグラム画像をゼロクリア
		histImage = cv::Mat::zeros(cv::Size(HISTIMAGE_WIDTH, HISTIMAGE_HEIGHT), CV_8UC3);
		binW = histImage.size().width / hdims;
		//	ヒストグラムを描画する
		for (i = 0; i < hdims; i++) {
			val = cv::saturate_cast<int>(hist.at<float>(i)*histImage.rows / 255);

			cv::Scalar color = hsv2rgb(i * 180.0 / hdims);
			cv::rectangle(histImage,
				cv::Point(i * binW, histImage.size().height),
				cv::Point((i + 1) * binW, histImage.size().height - val),
				color,
				-1, 8);
		}
	}
	return hist;
}




int
main(int argc, char *argv[])
{
	char		windowNameHistogram[] = "Histogram";			//	ヒストグラムを表示するウインドウ名
	char		windowNameObjectTracking[] = "ObjectTracking";	//	物体追跡結果を表示するウインドウ名

	cv::Mat frameImage;         //	キャプチャ画像用cv::Mat


	//cv::VideoCapture capWebcam(0);		// declare a VideoCapture object and associate to webcam, 0 => use 1st webcam

	cv::VideoCapture capWebcam("http://172.16.0.254:9176");  
	//This Http address allows you to capture a stream from Kodak SP360 through WIFI

	capWebcam.set(CV_CAP_PROP_FRAME_WIDTH, 720);
	capWebcam.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
	//  int	i;
	//	int	j = 0;
	int		key;	//	キー入力結果を格納する変数

					//    cv::Point pt[SEGMENT];	//	制御点の座標
	cv::Size window;			//	最小値を探索する近傍サイズ
	window.width = WINDOW_WIDTH;
	window.height = WINDOW_HEIGHT;
	cv::TermCriteria crit;
	crit.type = CV_TERMCRIT_ITER;		//	終了条件の設定
	crit.maxCount = ITERATION_SNAKE;	//	関数の最大反復数

	cv::Point_<float> vtx[4];   //for drawing lines

								//	カメラを初期化する

	if (!capWebcam.isOpened()) {
		//	カメラが見つからなかった場合
		printf("カメラが見つかりません\n");
		return -1;
	}

	std::cout << "Hot keys: \n"
		"\tq - quit the program\n"
		"\tc - stop the tracking\n"
		"\tb - switch to/from backprojection view\n"
		"\th - show/hide object histogram\n"
		"To initialize tracking, select the object with mouse\n";

	//	ウインドウを生成する
	cv::namedWindow(windowNameHistogram, CV_WINDOW_AUTOSIZE);
	cv::namedWindow(windowNameObjectTracking, CV_WINDOW_AUTOSIZE);

	//	トラックバーを生成する
	cv::setMouseCallback(windowNameObjectTracking, on_mouse, 0);

	cv::createTrackbar("Vmin", windowNameObjectTracking, &vmin, 256, 0);
	cv::createTrackbar("Vmax", windowNameObjectTracking, &vmax, 256, 0);
	cv::createTrackbar("Smin", windowNameObjectTracking, &vmin, 256, 0);

	while (1) {
		//	キャプチャ画像の取得に失敗したら処理を中断
		capWebcam >> frameImage;
		if (frameImage.empty()) {
			break;
		}
		//	キャプチャに成功したら処理を継続する
		if (resultImage.empty()) {
			//	各種画像の確保
			resultImage = cv::Mat(frameImage.size().width, frameImage.size().height, CV_8UC3);

			hsvImage = cv::Mat(frameImage.size(), CV_8UC3);
			hueImage = cv::Mat(frameImage.size(), CV_8UC1);
			maskImage = cv::Mat(frameImage.size(), CV_8UC1);
			backprojectImage = cv::Mat(frameImage.size(), CV_8UC1);
			grayImage = cv::Mat(frameImage.size(), CV_8UC1);

			//	ヒストグラム用の画像を確保し、ゼロクリア
			histImage = cv::Mat::zeros(cv::Size(HISTIMAGE_WIDTH, HISTIMAGE_HEIGHT), CV_8UC3);
		}
		//	キャプチャされた画像をresultImageにコピーし、HSV表色系に変換してhsvImageに格納
		resultImage = frameImage.clone();
		cv::cvtColor(resultImage, hsvImage, CV_BGR2HSV);

		//	trackObjectフラグがTRACKING_STOP以外なら、以下の処理を行う
		if (trackObject != TRACKING_STOP) {

			//追跡領域のヒストグラム計算と描画
			hist = CalculateHist(hist, hsvImage, maskImage, selection);

			//	バックプロジェクションを計算する
			cv::calcBackProject(&hueImage, 1, hChannels, hist, backprojectImage, histRanges);
			//	backProjectionのうち、マスクが1であるとされた部分のみ残す
			cv::bitwise_and(backprojectImage, maskImage, backprojectImage);

			if (trackWindow.width == 0 || trackWindow.height == 0) {
				trackObject = TRACKING_STOP;
				continue;
			}
			//	CamShift法による領域追跡を実行する
			trackRegion = cv::CamShift(backprojectImage,
				trackWindow,
				cv::TermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1));
			//  trackWindow's being zero-width or zero-height may cause error

			//	SnakeImage用のグレースケール画像を作成する
			cv::cvtColor(resultImage, grayImage, CV_BGR2GRAY);

			if (backprojectMode == SHOW_BACKPROJECTION) {
				cv::cvtColor(backprojectImage, resultImage, CV_GRAY2BGR);
			}

			//	CamShiftでの領域追跡結果をSnakeの初期位置に設定する
			//			for( i=0; i<SEGMENT; i++ ){
			////				pt[i].x = cvRound(	trackRegion.size.width
			////                                  * cos(i * 6.28 / SEGMENT + trackRegion.angle)
			////                                  / 2.0 + trackRegion.center.x );
			//                pt[i].x = round(    trackRegion.size.width
			//                                    * cos(i * 6.28 / SEGMENT + trackRegion.angle)
			//                                    / 2.0 + trackRegion.center.x );
			////				pt[i].y = cvRound(	trackRegion.size.height
			////                                  * sin(i * 6.28 / SEGMENT + trackRegion.angle)
			////                                  / 2.0 + trackRegion.center.y );
			//                pt[i].y = round(    trackRegion.size.height
			//                                    * sin(i * 6.28 / SEGMENT + trackRegion.angle)
			//                                    / 2.0 + trackRegion.center.y );
			//			}
			//	Snakeによる輪郭抽出を実行する -> not available for 2.4.9

			//  trackRegionの四角の線を描画
			trackRegion.points(vtx);
			std::cout << trackRegion.center;
			for (int i = 0; i<4; i++)
				cv::line(resultImage, vtx[i], vtx[i<3 ? i + 1 : 0], cv::Scalar(100, 100, 200), 2, CV_AA);
			//  trackRegionに内接する楕円を描画
			cv::ellipse(resultImage, trackRegion, cv::Scalar(100, 100, 200), 2, CV_AA);
			//  trackWindowを描画
			cv::rectangle(resultImage, trackWindow, cv::Scalar(0, 200, 0), 2, CV_AA);

			//  トラッキング開始時の画像をファイルに出力するためのコード
			//            if (!j){
			//                j=1;
			//                cv::imwrite("result.png", resultImage);
			//                cv::imwrite("hist.png", histImage);
			//                cv::imwrite("backproject.png", backprojectImage);
			//            }

		}
		//	マウスで選択中の初期追跡領域の色を反転させる
		if (selectObject == SELECT_ON && selection.width > 0 && selection.height > 0) {
			cv::Mat roi(resultImage, selection);
			bitwise_not(roi, roi);
		}


		//	画像を表示する
		cv::imshow(windowNameObjectTracking, resultImage);
		if (showHist)
			cv::imshow(windowNameHistogram, histImage);

		//	キー入力を待ち、押されたキーによって処理を分岐させる
		key = cv::waitKey(10);
		if ((char)key == 'q')
			//	while無限ループから脱出（プログラムを終了）
			break;
		switch ((char)key) {
		case 'b':
			//	表示画像をバックプロジェクション画像に切り替える
			if (backprojectMode == HIDDEN_BACKPROJECTION) {
				backprojectMode = SHOW_BACKPROJECTION;
			}
			else {
				backprojectMode = HIDDEN_BACKPROJECTION;
			}
			break;
		case 'c':
			//	トラッキングを中止する
			trackObject = TRACKING_STOP;
			histImage = cv::Mat::zeros(HISTIMAGE_WIDTH, HISTIMAGE_HEIGHT, CV_8UC3);
			break;
		case 'h':
			//	ヒストグラムの表示/非表示を切り替える
			if (showHist == HIDDEN_HISTOGRAM) {
				showHist = SHOW_HISTOGRAM;
			}
			else {
				showHist = HIDDEN_HISTOGRAM;
			}
			if (!showHist) {
				cv::destroyWindow(windowNameHistogram);
			}
			else {
				cv::namedWindow(windowNameHistogram, CV_WINDOW_AUTOSIZE);
			}
			break;
		default:
			;
		}
	}

	return 0;
}
