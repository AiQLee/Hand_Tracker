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
�h���b�O����ƃg���b�L���O�J�n�B
q - �v���O�����I��
c - �g���b�L���O�I��
b - �o�b�N�v���W�F�N�V�����\���̐؂�ւ�
h - �q�X�g�O�����\���̃I���I�t
*/

#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>

#define		SEGMENT				150		//	cvSnakeImage�ŗp���鐧��_�̐�
#define		WINDOW_WIDTH		17		//	cvSnakeImage�ōŏ��l��T������ߖT�̈�̕�
#define		WINDOW_HEIGHT		17		//	cvSnakeImage�ōŏ��l��T������ߖT�̈�̍���

cv::Mat	resultImage;			//	�������ʕ\���pcv::Mat
cv::Mat	hsvImage;               //	HSV�\�F�n�pcv::Mat
cv::Mat	hueImage;               //	HSV�\�F�n��H�`�����l���pcv::Mat
cv::Mat	maskImage;              //	�}�X�N�摜�pcv::Mat
cv::Mat	backprojectImage;       //	�o�b�N�v���W�F�N�V�����摜�pcv::Mat
cv::Mat	histImage;              //	�q�X�g�O�����`��pcv::Mat
cv::Mat	grayImage;              //	�O���[�X�P�[���摜�pcv::Mat

cv::Mat hist;                   //	�q�X�g�O���������pcv::Mat


//	CamShift�g���b�L���O�p�ϐ�
cv::Point			origin;
cv::Rect			selection;
cv::Rect			trackWindow;
cv::RotatedRect     trackRegion;


int
main(int argc, char *argv[])
{
	char		windowNameHistogram[] = "Histogram";			//	�q�X�g�O������\������E�C���h�E��
	char		windowNameObjectTracking[] = "ObjectTracking";	//	���̒ǐՌ��ʂ�\������E�C���h�E��
	char		windowObjectT[] = "ObjectT";

	cv::Mat frameImage;         //	�L���v�`���摜�pcv::Mat
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
	int		key;	//	�L�[���͌��ʂ��i�[����ϐ�


								//	�J����������������

	if (!capWebcam.isOpened()) {
		//	�J������������Ȃ������ꍇ
		printf("�J������������܂���\n");
		return -1;
	}
	//	�E�C���h�E�𐶐�����
	cv::namedWindow(windowNameObjectTracking, CV_WINDOW_AUTOSIZE);
	//cv::namedWindow(windowObjectT, CV_WINDOW_AUTOSIZE);

	while (1) {
		//	�L���v�`���摜�̎擾�Ɏ��s�����珈���𒆒f
		capWebcam >> frameImage;
		//capWebcam1 >> frameImage1;

		if (frameImage.empty()) {
			break;
		}

		//	�摜��\������

		cv::Mat	rawImage = cv::Mat(600, 800, CV_8UC3);
		cv::resize(frameImage, rawImage, rawImage.size());

		cv::Mat	resultImage = cv::Mat(600, 800, CV_8UC3);

		for (int i = 0; i < 800; i++) {
			rawImage.col(i).copyTo(resultImage.col(799-i));
		}

		cv::imshow(windowNameObjectTracking, resultImage);
		//cv::imshow(windowObjectT, frameImage1);


		//	�L�[���͂�҂��A�����ꂽ�L�[�ɂ���ď����𕪊򂳂���
		key = cv::waitKey(10);
	
	}

	return 0;
}
