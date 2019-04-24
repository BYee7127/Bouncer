/**
 * Bouncer file takes in an image and draws a bouncing ball over it.
 * The ball needs to have some sort of shading/gradient on it and the
 * bounce must be reasonable.
 *
 * Date:   2019.03.27
 * Author: Kade Challis, Beverly Yee
 **/

#include <iostream>
#include <cmath>

// allows the inclusion of C code files in C++
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/dict.h>
#include <libavutil/imgutils.h>
}

using namespace std;

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame);
void overlay_ball(AVFrame *pFrame, int width, int height, int j);

int main (int argc, char ** argv)
{
  const AVCodec *codec;
  AVFormatContext *pFormatCtx = NULL;
  const char* filename = argv[1];
  int vidStream, i, numBytes, frameFinished, ret, x, y;
  uint8_t *buffer = NULL;
  AVCodecContext *tempCtx;
  AVCodecContext *codecCtx = avcodec_alloc_context3(NULL);
  AVCodecParameters *param = avcodec_parameters_alloc();
  AVStream *stream = NULL;
  AVFrame *frame, *RGBframe = NULL;
  struct SwsContext *sws_ctx = NULL;
  AVPacket *packet;

  // Checks to make sure the code we're using exists
  codec = avcodec_find_encoder_by_name("cool");
  if (!codec) {
    av_log(codecCtx, AV_LOG_INFO, "cool codec not found\n");
    exit(1);
  }
  else {
    // codec is found, so check the file name
    av_log(codecCtx, AV_LOG_INFO, "cool codec found. Processing filename.\n");

    // for ease, make the char* array into a string
    string input(filename);
    string jpg(".jpg");
    int find = input.find(jpg);

    if(find == string::npos){
      // the find function returns a negative number if it cannot find the
      // aforementioned string. If that happens, the extension/file is not
      // a JPG - exit the application
      av_log(codecCtx, AV_LOG_INFO, "Invalid file format: Not a JPG file.\n");
      exit(1);
    }
  }


  /** Open video file - function only looks at header of file
    * Filename is the file. pFormatCtx is the pointer to the format context
    * of the file. The first NULL argument is the parameter for file format
    * - FFMPEG autmatically denotes that if it's NULL, forces for any other
    * argument. The second NULL argument is for an options parameter. 
    * http://dranger.com/ffmpeg/tutorial01.html **/
  if(avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0)
    return -1; // Couldn't open file

  // Retrieve stream information
  // http://dranger.com/ffmpeg/tutorial01.html
  if(avformat_find_stream_info(pFormatCtx, NULL) < 0)
    return -1; // Couldn't find stream information

  // Dump information about file
  av_dump_format(pFormatCtx, 0, filename, 0);

  /** Walk through the stream of pFormatCtx pointer array to find the video
    * stream - codec is deprecated, replace with codecpar
    * http://libav-users.943685.n4.nabble.com/Libav-user-AVStream-gt-codec-deprecation-td4662437.html **/
  vidStream = -1;
  for (i = 0; i < pFormatCtx->nb_streams; i++){
    stream = pFormatCtx->streams[i];
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      vidStream = i;
      break;
    }
  }

  if (vidStream == -1){
    av_log(tempCtx, AV_LOG_ERROR, "Could not find a video stream\n");
    return AVERROR_INVALIDDATA;
  }

  // Get a pointer to the codec context for the video stream
  avcodec_parameters_to_context(codecCtx, stream->codecpar);

  // find the decoder for the vid stream - changed output function to reflect
  // FFMPEG output function. 
  // http://dranger.com/ffmpeg/tutorial01.html
  codec = avcodec_find_decoder(codecCtx->codec_id);
  if(codec == NULL) {
    av_log(codecCtx, AV_LOG_ERROR, "Unsupported codec\n");
    return AVERROR_INVALIDDATA;
  }

  // copy context - changed output function to reflect FFMPEG output function
  // copy function deprecated - used a different function that returns a NULL upon
  // failure
  // http://dranger.com/ffmpeg/tutorial01.html
  tempCtx = avcodec_alloc_context3(codec);

  /** codec is deprecated and codecpar is of type AVCodecParameter, not
    * AVCodecContex, which pCodecCtx is. Convert.
    * https://ffmpeg.org/pipermail/libav-user/2016-October/009801.html 
    * https://stackoverflow.com/questions/39536746/ffmpeg-leak-while-reading-image-files **/
  // Fill the parameters struct based on the values from the supplied codec context.
  avcodec_parameters_from_context(param, codecCtx);
  // Fill the codec context based on the values from the supplied codec parameters.
  avcodec_parameters_to_context(tempCtx, stream->codecpar);

  // start of code from http://dranger.com/ffmpeg/tutorial01.html
  if (tempCtx == NULL){
    av_log(tempCtx, AV_LOG_ERROR, "Could not copy codec context.\n");
    return AVERROR_INVALIDDATA;
  }

  // open codec
  if (avcodec_open2(tempCtx, codec, NULL) < 0){
    av_log(codecCtx, AV_LOG_ERROR, "Could not open codec.\n");
    return AVERROR_INVALIDDATA;
  }

  // allocate video frame
  frame = av_frame_alloc();
  RGBframe = av_frame_alloc();
  if (RGBframe == NULL || frame == NULL){
    av_log(codecCtx, AV_LOG_ERROR, "Could not allocate frame.\n");
    return AVERROR_INVALIDDATA;
  }

  // determine the required buffer size and allocate it
  // avpicture_get_size deprecated
  // https://mail.gnome.org/archives/commits-list/2016-February/msg05531.html
  numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, tempCtx->width,
				      tempCtx->height, 1);
  buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

  // assign parts of buffer to image planes in RGB frame
  av_image_fill_arrays(RGBframe->data, RGBframe->linesize, buffer, AV_PIX_FMT_RGB24, 
  tempCtx->width, tempCtx->height, 1);

  packet = av_packet_alloc();


  // Time to read from stream! Yay.
  // initialize SWS context for software scaling
  sws_ctx = sws_getContext(tempCtx->width,
			   tempCtx->height,
			   tempCtx->pix_fmt,
			   tempCtx->width,
			   tempCtx->height,
			   AV_PIX_FMT_RGB24,
			   SWS_BILINEAR,
			   NULL, NULL, NULL);

  i = 0;
  while (av_read_frame(pFormatCtx, packet) >= 0) {
    // check if the packet is from the video stream
    if (packet->stream_index == vidStream) {
      // avcodec_decode_video2 deprecated
      // send for encoding
      // avcodec_send_packet(tempCtx, packet);
       // receive a packet for encoding (?)
      // frameFinished = avcodec_receive_frame(tempCtx, RGBframe);
      avcodec_decode_video2(tempCtx, frame, &frameFinished, packet);

      // check for video frame
      if (frameFinished) {
	for(int j = 0; j < 10; j++){
	  // convert image
	  sws_scale(sws_ctx, (uint8_t const * const *)frame->data,
		    frame->linesize, 0, tempCtx->height, RGBframe->data,
		    RGBframe->linesize);
	
	  overlay_ball(RGBframe, tempCtx->width, tempCtx->height, j);

	  // save to disk
	  SaveFrame(RGBframe, tempCtx->width, tempCtx->height, j);
	}
      }
    }

      // free the packet - av_free_packet is deprecated
      av_packet_unref(packet);
  }

  // Free the RGB image
  av_free(buffer);
  av_free(RGBframe);

  // Free the YUV frame
  av_free(frame);
 
  // Close the codecs
  avcodec_close(tempCtx);
  avcodec_close(codecCtx);
 
  // Close the video file
  avformat_close_input(&pFormatCtx);

  return 0;
// end of code from http://dranger.com/ffmpeg/tutorial01.html
}

/**
 * SaveFrame function writes the RGB information into whatever format
 * we're using. In the tutorial, they use PPM. For our purposes, we are
 * using the cool format.
 * Taken from http://dranger.com/ffmpeg/tutorial01.html
 **/
void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
  FILE *pFile;
  char szFilename[32];
  int  y;
  
  // Open file
  sprintf(szFilename, "frame%i.cool", iFrame);
  pFile = fopen(szFilename, "wb");
  if(pFile == NULL)
    return;
  
  // Write header
  fprintf(pFile, "P6\n%d %d\n255\n", width, height);
  
  // Write pixel data
  for(y = 0; y < height; y++)
    fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
  
  // Close file
  fclose(pFile);
}

/**
 * Drawing the ball over the frame of the current image
 **/
void overlay_ball (AVFrame * pFrame, int width, int height, int j)
{
  int radius = 15;

  // find center of frame and draw ball relative to center
  int x_center = 0 + radius;
  int y_center = 0 + radius;

  // how much of the movement of the circle is
  int move_x = 30*j;
  int move_y = 30*j;

  // iterate over pixels in frame and draw a circle
  for(int y = 0; y < height; y++){
    for(int x = 0; x < width; x++){
      // center of the circle
      int x_circ = pow((x-move_x) - x_center, 2);
      int y_circ = pow((y-move_y) - y_center, 2);
      //x_circ = x_circ + move_x;
      //y_circ = y_circ + move_y;

      //cout << "x2 = " << x2 << ", y2 = " << y2 << "\n";

      // if the pixel is within a certain range, draw
      int offset = 3*(x + y*width);
      if((x_circ + y_circ) <= (pow(radius,2))){
	  pFrame->data[0][offset+0] = 0;
	  pFrame->data[0][offset+1] = 255;
	  pFrame->data[0][offset+2] = 0;
      }

      if(x_circ + radius > pow(width, 2)){
	//cout << "x_circ = " << x_circ << ", width = " << width << "\n";
	x_circ = width-radius;
	move_x = move_x * (-1);
      }
      if(y_circ + radius < pow(height, 2)){
	y_circ = height - radius;
	move_y = move_y * (-1);
      }
    }
  }
}
