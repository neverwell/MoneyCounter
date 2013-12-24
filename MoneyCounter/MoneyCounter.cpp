#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <stdlib.h>
#include <iostream>

/****
�Ȳ���һ������ֵ��ֵ������ͼ��Ȼ�������ͺ�ʴȥ�������ĵ㲢��ͼ���ڲ�Ū��һ�����壬
Ȼ�����δ��ϵ��´�����ɨ�赽Ϊ255�ĵ㣬���ҳ��߽磬�����ͻ����˾��ο�Ϊ���ҵ�׼ȷ�ĽǶȣ�
ʹ��hough�任�ҵ�ֱ�ߣ�����¼�½Ƕ�
Ȼ����ҵ��ĽǶȽ��м򵥵�ͳ�ƣ��ҵ����ִ������ĽǶȣ��������ֽ�ҵĽǶ�
Ȼ���ݽǶ�����ֽ��ͼ�������ת
***/

using namespace std;
using namespace cv;

struct angle_count{
  float angle;//�Ƕ�
  int   nCount;//���ִ���
};

/// Global variables

int threshold_value = 0;
int threshold_type = 3;;
int const max_value = 255;
int const max_type = 4;
int const max_BINARY_value = 255;

Mat src, src_gray, dst;
char* window_name = "Threshold Demo";

char* trackbar_type = "Type: \n 0: Binary \n 1: Binary Inverted \n 2: Truncate \n 3: To Zero \n 4: To Zero Inverted";
char* trackbar_value = "Value";

/// ��������ɨ���ҵ�ֱ��
Mat FindLine(Mat I);
///hough�任�ҵ�ֱ��
void DoHough(Mat& dst);

/**
* @function main
*/
int main( int argc, char** argv )
{
	char resultfilename[_MAX_PATH];
	// strcpy(resultfilename,argv[1]);
	memset(resultfilename,0,_MAX_PATH);
	strncpy(resultfilename,argv[1],strlen(argv[1])-4);
	sprintf(resultfilename,"%s_res.bmp",resultfilename);
	/// Load an image
	src = imread( argv[1], 1 );




	/// Convert the image to Gray
	cvtColor( src, src_gray, CV_RGB2GRAY );

	/// Create a window to display results
	namedWindow( window_name, CV_WINDOW_AUTOSIZE );

	/* 0: Binary
	1: Binary Inverted
	2: Threshold Truncated
	3: Threshold to Zero
	4: Threshold to Zero Inverted
	*/
	threshold_type=0;//��ֵ��
	threshold_value=10;//��ֵ��ֵ
	//��ֵ��
	threshold( src_gray, dst, threshold_value, max_BINARY_value,threshold_type );

	//�����ͺ�ʴ���ѹ����ĵ㶼����
	int erosion_elem = 0;
	int erosion_size = 2;
	int dilation_elem = 0;
	int dilation_size = 2;
	int const max_elem = 2;
	int const max_kernel_size = 21;
	int erosion_type;
	if( erosion_elem == 0 ){ erosion_type = MORPH_RECT; }
	else if( erosion_elem == 1 ){ erosion_type = MORPH_CROSS; }
	else if( erosion_elem == 2) { erosion_type = MORPH_ELLIPSE; }
	Mat element = getStructuringElement( erosion_type,
		Size( 2 * erosion_size + 1, 2 * erosion_size+1 ),
		Point( erosion_size, erosion_size ) );
	///Apply the erosion operation
	Mat erosion_dst,dilation_dst;
	dilate( dst, erosion_dst, element );
	erode( erosion_dst, dilation_dst, element );


	//ֱ���ҵ��ĸ���
	Mat show_binary=FindLine(dilation_dst);
	imshow("edge",show_binary);

	//����Hough�任�ҵ����е�ֱ��,ֻ���ĸ�
	DoHough(show_binary);

	//cout<<dst;

	imwrite(resultfilename,dst);
	imshow( window_name, show_binary );

	waitKey( 0 );

}


Mat FindLine(Mat I)
{
	// accept only char type matrices
	CV_Assert(I.depth() != sizeof(uchar));
	//����һ��ͼ����󣬳�ʼ��Ϊ0
	Mat show_binary=Mat::zeros(src.rows,src.cols,CV_8U);
	int channels = I.channels();
	int nRows = I.rows;
	int nCols = I.cols*channels;
#if 0
	if (I.isContinuous())
	{
		nCols *= nRows;
		nRows = 1;
	}

#endif 
	int i,j;
	bool flag1=false,flag2=false;
	uchar *p,*ps;
	//���������ߵı�
	for( i = 0; i < nRows; ++i)
	{
		p = I.ptr<uchar>(i);
		ps= show_binary.ptr<uchar>(i);
		flag1=false;
		flag2=false;
		for ( j = 0; j < nCols; ++j)
		{
			if(p[j]==255&&(!flag1))
			{//����ߵı�
				ps[j]=255;
				flag1=true;
			}
			if(p[j]==0&&flag1&&(!flag2))
			{//���ұߵı�
				ps[j]=255;
				flag2=true;
				break;
			}

		}

	}
	//���������ߵı�
	for ( i = 0; i < nCols; i++)
	{

		flag1=false;
		flag2=false;
		for ( j = 0; j < nRows; j++)
		{
			p = I.ptr<uchar>(j);
			ps= show_binary.ptr<uchar>(j);
			if(p[i]==255&&(!flag1))
			{//���ϱߵı�
				ps[i]=255;
				flag1=true;
			}
			if(p[i]==0&&flag1&&(!flag2))
			{//���±ߵı�
				ps[i]=255;
				flag2=true;
				break;
			}

		}
	}
	return show_binary;
}

void DoHough(Mat& dst)
{//����任
	vector<Vec2f> lines;
	vector<float> vertical_angle;
	vector<float> horizontal_angle;
	vector<float> angles;


	HoughLines(dst, lines, 1, CV_PI/180, 100, 0, 0 );
	struct angle_count *angle_struct=new struct angle_count[lines.size()];
	memset(angle_struct,lines.size(),sizeof(struct angle_count));
	int nnn=0;
	bool flag=false;
	//���
	dst=Mat::zeros(dst.rows,dst.cols,CV_8U);
	for( size_t i = 0; i < lines.size(); i++ )
	{
		float rho = lines[i][0], theta = lines[i][1];
		Point pt1, pt2;
		double a = cos(theta), b = sin(theta);
		double x0 = a * rho, y0 = b * rho;
		//��¼��ֱֱ�ߵĽǶȺ�ˮƽֱ�ߵĽǶ�
		angles.push_back(theta);
		flag=false;
		for (int j = 0; j < nnn; j++)
		{
			if(abs(theta-angle_struct[j].angle)<0.001)
			{
				flag=true;
				angle_struct[j].nCount+=1;
			}
		}
		if(!flag)
		{
			angle_struct[nnn].angle=theta;
			angle_struct[nnn].nCount=1;
			nnn++;
		}

		pt1.x = cvRound(x0 + 1000 * (-b));
		pt1.y = cvRound(y0 + 1000 * (a));
		pt2.x = cvRound(x0 - 1000 * (-b));
		pt2.y = cvRound(y0 - 1000 * (a));
		line( dst, pt1, pt2, Scalar(255,0,0), 1, CV_AA);
		cout<<theta<<endl;
		//cout<<vertical_angle[i]<<endl;
		//break;
	}

	//�ҵ����ĽǶ�
	int max=0;
	float perfact_angle=0.0;
	for (int i = 0; i < nnn; i++)
	{
		if(angle_struct[i].nCount>max)
		{
		   max=angle_struct[i].nCount;
		   perfact_angle=angle_struct[i].angle;
		}
	}
	cout<<endl<<endl<<perfact_angle<<endl;

}


