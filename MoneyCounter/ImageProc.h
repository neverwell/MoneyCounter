#ifndef  _DAL_IMAGE_PROC_H_
#define  _DAL_IMAGE_PROC_H_

#include<windows.h>

BYTE *Read8BitBmpFile2Img(const TCHAR * filename,int *width,int *height);
bool Write8BitImg2BmpFile(BYTE *pImg,int width,int height,const TCHAR * filename);
BOOL Write24bitBmpToImg(BYTE *pImg,int width,int height,const TCHAR * filename);

//��8bit�ĻҶ�ͼӳ�䵽24bit�ĻҶȿռ���
void GryImgToRGBImg(BYTE *pGryImg1,BYTE *pGryImg2,BYTE *pGryImg3,int width,int height,BYTE *pRGBImg);
//ͼ��ֱ��ת����
void FlipVertical(BYTE *pImg,bool isRGB,int width,int height);
//��cis
void TranslateUpCisImg(BYTE *src,BYTE *dst,int width,int height,int mode=1);


//Sobel����




#endif //_DAL_IMAGE_PROC_H_