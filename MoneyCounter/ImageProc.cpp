#include "ImageProc.h"
#include <stdio.h>
#include<TCHAR.H>

BYTE *RmwRead8BitBmpFile2Img(const TCHAR * filename,int *width,int *height)
{
    //读取8Bit灰度图像，文件格式是：bmp
	FILE *BinFile;
	BITMAPFILEHEADER FileHeader;
	BITMAPINFOHEADER BmpHeader;
	BYTE *pImg;
	unsigned int size;
	int Suc=1,w,h;//w,h是临时的变量，Suc是操作是否成功的

	//Open File
	*width=*height=0;
	_tfopen_s(&BinFile,filename,TEXT("rb"));//必须是二进制方式打开，这样比较好
	if(BinFile==NULL) return NULL;
	//Read Struct Info
	if(fread((void *)&FileHeader,1,sizeof(FileHeader),BinFile)!=sizeof(FileHeader)) Suc=-1;
	if(fread((void *)&BmpHeader,1,sizeof(BmpHeader),BinFile)!=sizeof(BmpHeader)) Suc=-1;
	if(Suc==-1||(FileHeader.bfOffBits<sizeof(FileHeader)+sizeof(BmpHeader)))
	{
	   fclose(BinFile);
	   return NULL;
	}
	//Read Image Data
	*width=w=(BmpHeader.biWidth+3)/4*4;
	*height=h=BmpHeader.biHeight;
	size=(BmpHeader.biWidth+3)/4*4*BmpHeader.biHeight;
	fseek(BinFile,FileHeader.bfOffBits,SEEK_SET);
	if((pImg=new BYTE[size])!=NULL)
	{
	  for(int i=0;i<h;i++)  //0,1,2,3,4(5):400-499
	  {
		  if(fread(pImg+(h-1-i)*w,sizeof(BYTE),w,BinFile)!=(unsigned int)w)
		  {
			  fclose(BinFile);
			  delete pImg;
			  pImg=NULL;
			  return NULL;
		  }
	  }	
	}
	fclose(BinFile);
	return pImg;
}


bool RmwWrite8BitImg2BmpFile(BYTE *pImg,int width,int height,const TCHAR * filename)
{
     //写8Bit灰度图像，文件格式为：bmp
	//注意，当宽度不足4的倍数时自动添加成4的倍数
	FILE *BinFile;
	BITMAPFILEHEADER FileHeader;
	BITMAPINFOHEADER BmpHeader;
	int i,extend;
	bool Suc=true;
	BYTE p[4],*pCur;
    //Create File
	_tfopen_s(&BinFile,filename,TEXT("w+b"));
	if(BinFile==NULL) { return false; };
    //Fill the FileHeader
	FileHeader.bfType=((WORD)('M'<<8)|'B');//bmp文件的默认的类别是"BM",而WORD类型存放的时候是先低后高的，所以存为MB了
	FileHeader.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+256*4L;
	FileHeader.bfSize=FileHeader.bfOffBits+width*height;
	FileHeader.bfReserved1=0;
	FileHeader.bfReserved2=0;
	if(fwrite((void *)&FileHeader,1,sizeof(FileHeader),BinFile)!=sizeof(FileHeader)) Suc=false;
	//Fill the ImgHeader
	BmpHeader.biSize=40;
	BmpHeader.biWidth=width;
	BmpHeader.biHeight=height;
	BmpHeader.biPlanes=1;
	BmpHeader.biBitCount=8;
	BmpHeader.biCompression=0;
	BmpHeader.biSizeImage=0;
	BmpHeader.biXPelsPerMeter=0;
	BmpHeader.biYPelsPerMeter=0;
	BmpHeader.biClrUsed=0;
	BmpHeader.biClrImportant=0;
	if(fwrite((void *)&BmpHeader,1,sizeof(BmpHeader),BinFile)!=sizeof(BmpHeader))  Suc=false;
	//write Pallete
	for (i=0,p[3]=0;i<256;i++)
	{
		p[3]=0;
		p[0]=p[1]=p[2]=i;
		if(fwrite((void *)p,1,4,BinFile)!=4) { Suc=false;break;};
	}
	//write image data
	extend=(width+3)/4*4-width;//计算出要填充的部分
	if(extend==0)
	{
	   for(pCur=pImg+(height-1)*width;pCur>=pImg;pCur-=width)//注意这里是pCur-width
	   {
	     if(fwrite((void *)pCur,1,width,BinFile)!=(unsigned int)width) Suc=false;//真实的数据
	   }
	}
	else
	{
	   for (pCur=pImg+(height-1)*width;pCur>=pImg;pCur-=width)
	   {
		   if(fwrite((void *)pCur,1,width,BinFile)!=(unsigned int)width) Suc=false;//真实的数据
		   for (i=0;i<extend;i++)
		   {
			   //此处是什么道理，没有明白，好像只填充了一个
			   if (fwrite((void *)(pCur+width-1),1,1,BinFile)!=1)  Suc=false;
		   }
	   }
	}
    //return 
	fclose(BinFile);
	return Suc;
}


BYTE *DAL_Read24bitFileToBmp(const TCHAR *filename,int *width,int *height)
{
    BITMAPFILEHEADER bfHeader;//位图文件头
	BITMAPINFOHEADER biHeader;//位图信息头
    //BITMAPINFO bmpinfo;//位图颜色表，24bit无此数据
	BYTE *pImg;
	int w,h,extend,i;
	char zero;
	FILE *fp;
	_tfopen_s(&fp,filename,TEXT("rb"));
	if(fp==NULL) return NULL;
	if(fread(&bfHeader,sizeof(BITMAPFILEHEADER),1,fp)!=1)
	{
		//printf("读取位图文件头出错！\n");
		fclose(fp);
		return NULL;
	}
	/*
	bfType里面存放的是BM，在windows中，bmp图片的bfType就是BM。BM是按ascii码存放的，分析如下：
	B=66=64+2=(1000010)2=(42)16
	M=77=64+8+4+1=（1001101）2=（4D）16
    DWORD（双字节）类型在内存中是先放低字节然后放高字节，所以'BM'在内存中是0x4d42 
	*/
	if (bfHeader.bfType!=0x4d42)
	{
		//printf("该文件不是bmp文件，无法打开！\n");
		fclose(fp);
		return NULL;
	}
	if(fread(&biHeader,sizeof(BITMAPINFOHEADER),1,fp)!=1)//fread返回的是读取的元素的个数，此处为1，下同
	{
		//printf("读取位图信息头出错！\n");
        fclose(fp);
		return NULL;
	}
	if (biHeader.biBitCount!=24)
	{
		//非24bit的bmp文件本函数无法操作
		//printf("该bmp图片不是24bit的，本函数无法打开！\n");
		fclose(fp);
		return NULL;
	}
	//注意，没有考虑位图压缩情况
	*width=w=biHeader.biWidth;//这里的宽度指的是像素数，24 bit的一个像素3个字节
	*height=h=biHeader.biHeight;
    pImg=(BYTE *)malloc(w*3*h*sizeof(BYTE));
	if (pImg==NULL)
	{
		//printf("为位图分配内存失败！\n");
		fclose(fp);
		return NULL;
	}
    for (i=0;i<h;i++)
    {
		//在内存中摆正bmp，即从最后一行开始读取，其实意义不大
		if (fread(pImg+(h-i-1)*w*3,w*3,1,fp)!=1)
		{
			delete pImg;
			//printf("读取bmp文件数据部分出错！\n");
			fclose(fp);
			return NULL;
		}
		//跳过windows为bmp图片填充的数据
	    extend=(w*3+3)/4*4-w*3;
		while(extend--)
			fread(&zero,sizeof(char),1,fp);
    }
	fclose(fp);
    return pImg;
}

BOOL DalWrite24bitBmpToImg(BYTE *pImg,int width,int height,const TCHAR * filename)
{
    //写24bit的bmp图片
    FILE *imgFile;
	_tfopen_s(&imgFile,filename,TEXT("wb"));
	if(imgFile==NULL) return FALSE;
	int w=((width*3+3)>>2)<<2;//w=(width+3)/4*4
	BITMAPFILEHEADER bfHeader;//位图文件头
	bfHeader.bfType=0x4D42;
	bfHeader.bfSize=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+width*height*3;
	bfHeader.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
	bfHeader.bfReserved1=0;
	bfHeader.bfReserved2=0;
	if(fwrite(&bfHeader,sizeof(BITMAPFILEHEADER),1,imgFile)!=1) 
	{
		fclose(imgFile);
		printf("写位图文件头失败!\n");
		return FALSE;
	};
	BITMAPINFOHEADER biHeader;//位图信息头
	biHeader.biBitCount=24;
	biHeader.biClrImportant=0;
	biHeader.biClrUsed=0;
	biHeader.biCompression=0;
	biHeader.biHeight=height;
	biHeader.biPlanes=1;
	biHeader.biSize=40;
	biHeader.biSizeImage=0;
	biHeader.biWidth=width;
	biHeader.biXPelsPerMeter=0;
	biHeader.biYPelsPerMeter=0;
	if(fwrite(&biHeader,sizeof(BITMAPINFOHEADER),1,imgFile)!=1)
	{
		fclose(imgFile);
		printf("写位图信息头失败!\n");
		return FALSE;
	};
	//写入位图数据，注意字节对齐
	int fill_count=w-width*3;
	if (fill_count==0)
	{
		for (BYTE *pCur=pImg+width*3*(height-1);pCur>=pImg;pCur-=width*3)
		{
			if(fwrite(pCur,sizeof(BYTE),width*3,imgFile)!=(unsigned int)(width*3))
			{
				fclose(imgFile);
				printf("写入位图数据出错！\n");
				return FALSE;
			}
		}
	}
	else
	{
		for (BYTE *pCur=pImg+width*3*(height-1);pCur>=pImg;pCur-=width*3)
		{
			if(fwrite(pCur,sizeof(BYTE),width*3,imgFile)!=(unsigned int)(width*3))
			{
				fclose(imgFile);
				printf("写入位图数据出错！\n");
				return FALSE;
			}
			if(fwrite(NULL,sizeof(BYTE),fill_count,imgFile)!=(unsigned int)(fill_count))
			{
				fclose(imgFile);
				printf("填充位图数据出错！\n");
				return FALSE;
			}
		}
	}
	fclose(imgFile);
	return true;
}


////////////////////////////////////////////////////////////////////////////////
//
//  将8bit的灰度图像填充成24bit的图像的函数，输入是三帧灰度图像，输出是24bit图像
//  
////////////////////////////////////////////////////////////////////////////////

void DAL_GryImgToRGBImg(BYTE *pGryImg1,BYTE *pGryImg2,BYTE *pGryImg3,int width,int height,BYTE *pRGBImg)
{
	BYTE *pCur1,*pCur2,*pCur3,*pCur,*pEnd;
	pCur=pRGBImg;
	pEnd=pRGBImg+width*height*3;
	pCur1=pGryImg1;
	pCur2=pGryImg2;
	pCur3=pGryImg3;

	for (;pCur!=pEnd;++pCur1,++pCur2,++pCur3)
	{
		*pCur=*pCur1;
		++pCur;
		*pCur=*pCur2;
		++pCur;
		*pCur=*pCur3;
		++pCur;
	}
   
	return ;
}

////////////////////////////////////////////////////////////////////////////////
//
//  图像垂直反转函数
//  
////////////////////////////////////////////////////////////////////////////////
void DAL_FlipVertical(BYTE *pImg,bool isRGB,int width,int height)
{
    if (isRGB)  return ;
    BYTE *pTemp=new BYTE[width];
	int i,j;
	for (i=0,j=height-1;i<height/2;++i,--j)
	{
      memcpy(pTemp,pImg+i*width,width);
	  memcpy(pImg+i*width,pImg+j*width,width);
	  memcpy(pImg+j*width,pTemp,width);
	}
    delete pTemp;
	return ;
}




////////////////////////////////////////////////////////////////////////////////
//
//  TranslateUpCisImg，转换函数：将上CIS图像信息翻译出来，一行反射光，一行透射光，保存在结果图片中
//  mode是模式，为1时是先存储偶数（从0开始）行，然后存储奇数行，为2时正好相反
//  
////////////////////////////////////////////////////////////////////////////////
void TranslateUpCisImg(BYTE *src,BYTE *dst,int width,int height,int mode)
{
   BYTE *pCur_src,*pCur_dst,*pEnd_src;
   //断言height宽度是2的整数倍，否则不向下执行
   // Assert(height%2==0);
   pEnd_src=src+width*height;
   pCur_dst=dst;
   if(mode==1)
   {
      //存储偶数行
      for(pCur_src=src;pCur_src<pEnd_src;pCur_src+=2*width,pCur_dst+=width)
      {
        memcpy(pCur_dst,pCur_src,width);
      }
      //存储奇数行
      for(pCur_src=src+width;pCur_src<pEnd_src;pCur_src+=2*width,pCur_dst+=width)
      {
        memcpy(pCur_dst,pCur_src,width);
      }
   }
   else 
   {
      //存储奇数行
	  for(pCur_src=src+width;pCur_src<pEnd_src;pCur_src+=2*width,pCur_dst+=width)
      {
        memcpy(pCur_dst,pCur_src,width);
      }
      //存储偶数行
	  for(pCur_src=src;pCur_src<pEnd_src;pCur_src+=2*width,pCur_dst+=width)
      {
        memcpy(pCur_dst,pCur_src,width);
      }
   }
}
