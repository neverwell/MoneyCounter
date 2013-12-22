#include "ImageProc.h"
#include <stdio.h>
#include<TCHAR.H>

BYTE *RmwRead8BitBmpFile2Img(const TCHAR * filename,int *width,int *height)
{
    //��ȡ8Bit�Ҷ�ͼ���ļ���ʽ�ǣ�bmp
	FILE *BinFile;
	BITMAPFILEHEADER FileHeader;
	BITMAPINFOHEADER BmpHeader;
	BYTE *pImg;
	unsigned int size;
	int Suc=1,w,h;//w,h����ʱ�ı�����Suc�ǲ����Ƿ�ɹ���

	//Open File
	*width=*height=0;
	_tfopen_s(&BinFile,filename,TEXT("rb"));//�����Ƕ����Ʒ�ʽ�򿪣������ȽϺ�
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
     //д8Bit�Ҷ�ͼ���ļ���ʽΪ��bmp
	//ע�⣬����Ȳ���4�ı���ʱ�Զ���ӳ�4�ı���
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
	FileHeader.bfType=((WORD)('M'<<8)|'B');//bmp�ļ���Ĭ�ϵ������"BM",��WORD���ʹ�ŵ�ʱ�����ȵͺ�ߵģ����Դ�ΪMB��
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
	extend=(width+3)/4*4-width;//�����Ҫ���Ĳ���
	if(extend==0)
	{
	   for(pCur=pImg+(height-1)*width;pCur>=pImg;pCur-=width)//ע��������pCur-width
	   {
	     if(fwrite((void *)pCur,1,width,BinFile)!=(unsigned int)width) Suc=false;//��ʵ������
	   }
	}
	else
	{
	   for (pCur=pImg+(height-1)*width;pCur>=pImg;pCur-=width)
	   {
		   if(fwrite((void *)pCur,1,width,BinFile)!=(unsigned int)width) Suc=false;//��ʵ������
		   for (i=0;i<extend;i++)
		   {
			   //�˴���ʲô����û�����ף�����ֻ�����һ��
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
    BITMAPFILEHEADER bfHeader;//λͼ�ļ�ͷ
	BITMAPINFOHEADER biHeader;//λͼ��Ϣͷ
    //BITMAPINFO bmpinfo;//λͼ��ɫ��24bit�޴�����
	BYTE *pImg;
	int w,h,extend,i;
	char zero;
	FILE *fp;
	_tfopen_s(&fp,filename,TEXT("rb"));
	if(fp==NULL) return NULL;
	if(fread(&bfHeader,sizeof(BITMAPFILEHEADER),1,fp)!=1)
	{
		//printf("��ȡλͼ�ļ�ͷ����\n");
		fclose(fp);
		return NULL;
	}
	/*
	bfType�����ŵ���BM����windows�У�bmpͼƬ��bfType����BM��BM�ǰ�ascii���ŵģ��������£�
	B=66=64+2=(1000010)2=(42)16
	M=77=64+8+4+1=��1001101��2=��4D��16
    DWORD��˫�ֽڣ��������ڴ������ȷŵ��ֽ�Ȼ��Ÿ��ֽڣ�����'BM'���ڴ�����0x4d42 
	*/
	if (bfHeader.bfType!=0x4d42)
	{
		//printf("���ļ�����bmp�ļ����޷��򿪣�\n");
		fclose(fp);
		return NULL;
	}
	if(fread(&biHeader,sizeof(BITMAPINFOHEADER),1,fp)!=1)//fread���ص��Ƕ�ȡ��Ԫ�صĸ������˴�Ϊ1����ͬ
	{
		//printf("��ȡλͼ��Ϣͷ����\n");
        fclose(fp);
		return NULL;
	}
	if (biHeader.biBitCount!=24)
	{
		//��24bit��bmp�ļ��������޷�����
		//printf("��bmpͼƬ����24bit�ģ��������޷��򿪣�\n");
		fclose(fp);
		return NULL;
	}
	//ע�⣬û�п���λͼѹ�����
	*width=w=biHeader.biWidth;//����Ŀ��ָ������������24 bit��һ������3���ֽ�
	*height=h=biHeader.biHeight;
    pImg=(BYTE *)malloc(w*3*h*sizeof(BYTE));
	if (pImg==NULL)
	{
		//printf("Ϊλͼ�����ڴ�ʧ�ܣ�\n");
		fclose(fp);
		return NULL;
	}
    for (i=0;i<h;i++)
    {
		//���ڴ��а���bmp���������һ�п�ʼ��ȡ����ʵ���岻��
		if (fread(pImg+(h-i-1)*w*3,w*3,1,fp)!=1)
		{
			delete pImg;
			//printf("��ȡbmp�ļ����ݲ��ֳ���\n");
			fclose(fp);
			return NULL;
		}
		//����windowsΪbmpͼƬ��������
	    extend=(w*3+3)/4*4-w*3;
		while(extend--)
			fread(&zero,sizeof(char),1,fp);
    }
	fclose(fp);
    return pImg;
}

BOOL DalWrite24bitBmpToImg(BYTE *pImg,int width,int height,const TCHAR * filename)
{
    //д24bit��bmpͼƬ
    FILE *imgFile;
	_tfopen_s(&imgFile,filename,TEXT("wb"));
	if(imgFile==NULL) return FALSE;
	int w=((width*3+3)>>2)<<2;//w=(width+3)/4*4
	BITMAPFILEHEADER bfHeader;//λͼ�ļ�ͷ
	bfHeader.bfType=0x4D42;
	bfHeader.bfSize=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+width*height*3;
	bfHeader.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
	bfHeader.bfReserved1=0;
	bfHeader.bfReserved2=0;
	if(fwrite(&bfHeader,sizeof(BITMAPFILEHEADER),1,imgFile)!=1) 
	{
		fclose(imgFile);
		printf("дλͼ�ļ�ͷʧ��!\n");
		return FALSE;
	};
	BITMAPINFOHEADER biHeader;//λͼ��Ϣͷ
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
		printf("дλͼ��Ϣͷʧ��!\n");
		return FALSE;
	};
	//д��λͼ���ݣ�ע���ֽڶ���
	int fill_count=w-width*3;
	if (fill_count==0)
	{
		for (BYTE *pCur=pImg+width*3*(height-1);pCur>=pImg;pCur-=width*3)
		{
			if(fwrite(pCur,sizeof(BYTE),width*3,imgFile)!=(unsigned int)(width*3))
			{
				fclose(imgFile);
				printf("д��λͼ���ݳ���\n");
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
				printf("д��λͼ���ݳ���\n");
				return FALSE;
			}
			if(fwrite(NULL,sizeof(BYTE),fill_count,imgFile)!=(unsigned int)(fill_count))
			{
				fclose(imgFile);
				printf("���λͼ���ݳ���\n");
				return FALSE;
			}
		}
	}
	fclose(imgFile);
	return true;
}


////////////////////////////////////////////////////////////////////////////////
//
//  ��8bit�ĻҶ�ͼ������24bit��ͼ��ĺ�������������֡�Ҷ�ͼ�������24bitͼ��
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
//  ͼ��ֱ��ת����
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
//  TranslateUpCisImg��ת������������CISͼ����Ϣ���������һ�з���⣬һ��͸��⣬�����ڽ��ͼƬ��
//  mode��ģʽ��Ϊ1ʱ���ȴ洢ż������0��ʼ���У�Ȼ��洢�����У�Ϊ2ʱ�����෴
//  
////////////////////////////////////////////////////////////////////////////////
void TranslateUpCisImg(BYTE *src,BYTE *dst,int width,int height,int mode)
{
   BYTE *pCur_src,*pCur_dst,*pEnd_src;
   //����height�����2������������������ִ��
   // Assert(height%2==0);
   pEnd_src=src+width*height;
   pCur_dst=dst;
   if(mode==1)
   {
      //�洢ż����
      for(pCur_src=src;pCur_src<pEnd_src;pCur_src+=2*width,pCur_dst+=width)
      {
        memcpy(pCur_dst,pCur_src,width);
      }
      //�洢������
      for(pCur_src=src+width;pCur_src<pEnd_src;pCur_src+=2*width,pCur_dst+=width)
      {
        memcpy(pCur_dst,pCur_src,width);
      }
   }
   else 
   {
      //�洢������
	  for(pCur_src=src+width;pCur_src<pEnd_src;pCur_src+=2*width,pCur_dst+=width)
      {
        memcpy(pCur_dst,pCur_src,width);
      }
      //�洢ż����
	  for(pCur_src=src;pCur_src<pEnd_src;pCur_src+=2*width,pCur_dst+=width)
      {
        memcpy(pCur_dst,pCur_src,width);
      }
   }
}
