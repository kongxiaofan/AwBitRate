#ifndef INFOR_H
#define INFOR_H

typedef enum{H265 = 1, H264, MPEG2, AVS}DecoderType;
typedef struct
{
    DecoderType de;
    int nFrameRate;
    bool isStreamType;
    int wigth;
    int heigth;
}Decoder;

#endif // INFOR_H
