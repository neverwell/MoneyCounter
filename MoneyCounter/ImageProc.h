#ifndef  _DAL_IMAGE_PROC_H_
#define  _DAL_IMAGE_PROC_H_

#include<windows.h>

BYTE *RmwRead8BitBmpFile2Img(const TCHAR * filename,int *width,int *height);
bool RmwWrite8BitImg2BmpFile(BYTE *pImg,int width,int height,const TCHAR * filename);
BOOL DalWrite24bitBmpToImg(BYTE *pImg,int width,int height,const TCHAR * filename);

//将8bit的灰度图映射到24bit的灰度空间中
void DAL_GryImgToRGBImg(BYTE *pGryImg1,BYTE *pGryImg2,BYTE *pGryImg3,int width,int height,BYTE *pRGBImg);
//图像垂直反转函数
void DAL_FlipVertical(BYTE *pImg,bool isRGB,int width,int height);
//上cis
void TranslateUpCisImg(BYTE *src,BYTE *dst,int width,int height,int mode=1);


#endif //_DAL_IMAGE_PROC_H_