#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <stdlib.h>
#include <iostream>

/****
先采用一定的阈值二值化整个图像，然后先膨胀后腐蚀去掉孤立的点并把图像内部弄成一个整体，
然后依次从上到下从左到右扫描到为255的点，即找出边界，这样就画出了矩形框，为了找到准确的角度，
使用hough变换找到直线，并记录下角度
然后对找到的角度进行简单的统计，找到出现次数最多的角度，这个就是纸币的角度
然根据角度来对纸币图像进行旋转
***/

/***********
不能用上面的方法找角度然后倾斜校正，因为最后钱币并不总是平行四边形
只有找到四条边，拟合出来然后根据某条边来进行倾斜校正，但是这样校正的也没有用
这校正的图像也不正，钱币不可能是正的，正的是理想情况下

版式识别99版和05版：
99版安全线细，05版安全线粗

面额识别：
找到了框之后进行填充，然后统计像素个数是进行面额识别的方法
另外面值不同安全线长度也不同，结合安全线长度来判断
10元和20元的面积是一样的，如果是这个就使用纸币颜色来进行鉴别，10元和20元的颜色不一样

残破识别：
折角或者缺角、孔洞是先用面积来判断是否有残破，把那个框里面的像素取反就看出来了，位置也定了

新旧识别：
梯度直方图
纸币水平投影垂直投影图像，投影相似性可以判断新旧

进行新旧识别的时候也不需要进行倾斜校正，为什么不需要倾斜校正
原因就是倾斜校正没有作用，纸币不可能总是平行四边形的

为了消除背景的影响可以画出框之后只在框里面的值求出直方图，不需要倾斜校正
可以一张二值的框填充后的图像和一张原始图像进行运算，二值图像255的地方才开始取
另外，小于20和大于230这部分丢弃，拿出100多维的数据进行训练
利用机器学习算法来进行处理

污字识别：

其他识别：

真伪鉴别：


**********/


/******
找四个点的算法，先找到四个边的数组，然后找分别找到四个端点，然后进行平均即可
//&&&这种算法不能应用于有折角或者缺角的纸币
//&&&对于那些纸币还是要找出直线来然后进行直线拟合求出交点即为纸币的四个点
*****/



using namespace std;
using namespace cv;

struct angle_count{
  float angle;//角度
  int   nCount;//出现次数
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

/// 上下左右扫描找到直线
Mat FindLine(Mat I);
///hough变换找到直线
float DoHough(Mat& dst);

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
	threshold_type=0;//二值化
	//threshold_value=10;//二值化值信比达的钱
	threshold_value=10;
	//二值化
	threshold( src_gray, dst, threshold_value, max_BINARY_value,threshold_type );
	imwrite("E:\\code\\MoneyCounter\\MoneyCounter\\Image\\0007.bmp",dst);

	//先膨胀后腐蚀，把孤立的点都消除
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

	Mat sobel_dst;
	Sobel(dilation_dst,sobel_dst,CV_8U,1,0);

	imshow("Sobel",sobel_dst);
	//直接找到四个边
	Mat show_binary=FindLine(dilation_dst);
	imshow("edge",show_binary);
	imwrite("E:\\code\\MoneyCounter\\MoneyCounter\\Image\\edge.bmp",show_binary);


	//利用Hough变换找到所有的直线,在多根直线的角度中找到最佳角度并返回
	float perfact_angle=DoHough(show_binary);

	//cout<<dst;

	imwrite(resultfilename,dst);
	imshow( window_name, show_binary );

	//根据最佳角度，对原始图像进行旋转
	Mat ttt_image=src.clone();
	


	waitKey( 0 );

}


Mat FindLine(Mat I)
{
	// accept only char type matrices
	CV_Assert(I.depth() != sizeof(uchar));
	//创建一个图像矩阵，初始化为0
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
	//找左右两边的边
	for( i = 0; i < nRows; ++i)
	{
		p = I.ptr<uchar>(i);
		ps= show_binary.ptr<uchar>(i);
		flag1=false;
		flag2=false;
		for ( j = 0; j < nCols; ++j)
		{
			if(p[j]==255&&(!flag1))
			{//找左边的边
				ps[j]=255;
				flag1=true;
			}
			if(p[j]==0&&flag1&&(!flag2))
			{//找右边的边
				ps[j]=255;
				flag2=true;
				break;
			}

		}

	}
	//找上下两边的边
	for ( i = 0; i < nCols; i++)
	{

		flag1=false;
		flag2=false;
		for ( j = 0; j < nRows; j++)
		{
			p = I.ptr<uchar>(j);
			ps= show_binary.ptr<uchar>(j);
			if(p[i]==255&&(!flag1))
			{//找上边的边
				ps[i]=255;
				flag1=true;
			}
			if(p[i]==0&&flag1&&(!flag2))
			{//找下边的边  不能这样扫过去，应该直接左右上下依次的扫，因为中间的部分不一定能膨胀腐蚀消失
				ps[i]=255;
				flag2=true;
				break;
			}

		}
	}
	return show_binary;
}

float DoHough(Mat& dst)
{//霍夫变换
	vector<Vec2f> lines;
	vector<float> vertical_angle;
	vector<float> horizontal_angle;
	vector<float> angles;


	HoughLines(dst, lines, 1, CV_PI/180, 100, 0, 0 );
	struct angle_count *angle_struct=new struct angle_count[lines.size()];
	memset(angle_struct,lines.size(),sizeof(struct angle_count));
	int nnn=0;
	bool flag=false;
	//清空
	dst=Mat::zeros(dst.rows,dst.cols,CV_8U);
	for( size_t i = 0; i < lines.size(); i++ )
	{
		float rho = lines[i][0], theta = lines[i][1];
		Point pt1, pt2;
		double a = cos(theta), b = sin(theta);
		double x0 = a * rho, y0 = b * rho;
		//记录垂直直线的角度和水平直线的角度
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
		//line( dst, pt1, pt2, Scalar(255,0,0), 1, CV_AA);
		cout<<theta<<endl;
		//cout<<vertical_angle[i]<<endl;
		//break;
	}

	//找到最佳角度
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
	//输出最佳角度
	cout<<endl<<endl<<perfact_angle<<endl;

	//画出最佳角度的边框


	return perfact_angle;

}


