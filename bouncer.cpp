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

void save_frame(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, int iFrame);
void overlay_ball(AVFrame *pFrame, int width, int height, int j);
int check_height(int j, int length, int height);
int check_width(int j, int length, int width);

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
    else{
      av_log(codecCtx, AV_LOG_INFO, "Valid file format. Continuing.\n");
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
    // returns a -1 if the AVMediaType is unknown. 0 is for Video
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
      // send for decoding
      avcodec_send_packet(tempCtx, packet);
       // receive a frame for encoding
      frameFinished = avcodec_receive_frame(tempCtx, frame);

      // check for video frame
      if (!frameFinished) {
        // if we entered here, that means we received the frame
        for(int j = 0; j < 300; j++){
          // convert image - RGBframe is the final decoded frame
          sws_scale(sws_ctx, (uint8_t const * const *)frame->data,
              frame->linesize, 0, tempCtx->height, RGBframe->data,
              RGBframe->linesize);

          RGBframe->width = tempCtx->width;
          RGBframe->height = tempCtx->height;

          // draw the ball over the image
          overlay_ball(RGBframe, tempCtx->width, tempCtx->height, j);

          // save to disk
          save_frame(tempCtx, RGBframe, packet, j);
        }
      }
    }

      // free the packet - av_free_packet is deprecated
      av_packet_unref(packet);
  }

  // Free the image
  av_free(buffer);
  av_free(RGBframe);
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
 * save_frame function writes the RGB information into whatever format
 * we're using. In the tutorial, they use PPM. For our purposes, we are
 * using the cool format.
 * Taken from http://dranger.com/ffmpeg/tutorial01.html
 * This function also relies heavily on the encode_video.c example in 
 * FFMPEG/docs/examples.
 **/
void save_frame(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, int iFrame) {
  FILE *pFile;
  char szFilename[32];
  int ret;  
  AVCodec *codec = avcodec_find_encoder_by_name("cool");
  AVCodecContext *enc_ctx = avcodec_alloc_context3(codec);

  enc_ctx->width = ctx->width;
  enc_ctx->height = ctx->height;
  /* frames per second */
  enc_ctx->time_base = (AVRational){1, 30};
  enc_ctx->framerate = (AVRational){30, 1};
  enc_ctx->pix_fmt = AV_PIX_FMT_RGB24;


  // Open file
  sprintf(szFilename, "frame%03d.cool", iFrame);
  pFile = fopen(szFilename, "wb");
  if(pFile == NULL)
    return;
    
    av_frame_get_buffer(frame, 1);
    avcodec_open2(enc_ctx, codec, NULL);

    /* send the frame to the encoder */
    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
      fprintf(stderr, "Error sending a frame for encoding\n");
      exit(1);
    }

    while (ret >= 0) {
      ret = avcodec_receive_packet(enc_ctx, pkt);
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
          return;
      else if (ret < 0) {
          fprintf(stderr, "Error during encoding\n");
          exit(1);
      }

      fwrite(pkt->data, 1, pkt->size, pFile);
      av_packet_unref(pkt);
    }

  // Close
  fclose(pFile);
  avcodec_close(enc_ctx);
}

/**
 * Drawing the ball over the frame of the current image
 **/
void overlay_ball (AVFrame * pFrame, int width, int height, int j){
  int length = 30;
  int radius = 15;  // messing of the radius uses a LOT of math, so keeping at 15

  // how much to move by, denoted by a number passed in
  int moveX = check_width(j, length, width);
  int moveY = check_height(j, length, height);

  for(int y = 0; y < height; y++){
   for(int x = 0; x < width; x++){
      // linesize - the *3 allows the RGB values to be set
      int offset = 3*(x + y*width);

      // make sure the circle moves, so add the move variables
      int circ_point = pow((x-moveX)-radius,2) + pow((y-moveY)-radius, 2);

      // start the shading!
      int light_shade_1 = pow((x-moveX)-radius-2,2) + pow((y-moveY)-radius+2, 2);
      int light_shade_2 = pow((x-moveX)-radius-3,2) + pow((y-moveY)-radius+3, 2);
      int light_shade_3 = pow((x-moveX)-radius-4,2) + pow((y-moveY)-radius+4, 2);

      // bounding box to make it easier to detect if the ball has hit the
      // edge of the frame
      if(x >= moveX && x <= moveX+length &&
	       y >= moveY && y <= moveY+length){
        // draw a circle within the square
        if(circ_point <= pow(radius, 2)){
          pFrame->data[0][offset+0] = 0;
          pFrame->data[0][offset+1] = 0;
          pFrame->data[0][offset+2] = 255;
        }
        
        // the shaded balls
        // no else if's or else the big circle overlays the others
        if(light_shade_1 <= pow(radius-3, 2)){
          // adding shades of red and green makes blue lighter
          pFrame->data[0][offset+0] = 50;
          pFrame->data[0][offset+1] = 50;
          pFrame->data[0][offset+2] = 255;
        }
        if(light_shade_2 <= pow(radius-8, 2)){
          pFrame->data[0][offset+0] = 80;
          pFrame->data[0][offset+1] = 80;
          pFrame->data[0][offset+2] = 255;
        }
        if(light_shade_3 <= pow(radius-12, 2)){
          pFrame->data[0][offset+0] = 150;
          pFrame->data[0][offset+1] = 150;
          pFrame->data[0][offset+2] = 255;
        }
      }
    }
  }
}

/**
 * Helper function for the overlay_ball function to make it easier to read
 * It checks whether the ball is within the bounds of the frame - height-wise.
 * The j parameter is the parameter passed in from the main function
 * The length parameter is the side length of the bounding box
 * The height parameter is the height of the frame
 **/
int check_height(int j, int length, int height){
  // how many pixels to move by
  int move = 17;

  // determines how many times a ball can cross the image before reaching an edge
  int frameCount = j % (height/move);

  // sets how much from the top/bottom the ball is moving
  int y = frameCount * move;

  // if the bounding box reaches the bottom, keep it there for a frame or two
  if(y+length >= height){
    y = height - length;
  }
  
  // this check is to see if the ball is bouncing back up
  if((j/(height/move)) % 2 == 1){
    // if it is, make the count negative
    frameCount = -frameCount;

    // and subtract it from the height
    // Don't forget the length - the ball will disappear mysteriously
    y = height - y - length;
  }

  return y;
}

/**
 * Similar to the function above, this function is a helper function to overlay_ball
 * to help it look better. This one checks to see whether the ball is within the
 * width-bounds of the frame.
 * The j parameter is the parameter passed by the main function
 * The length parameter is the side length of the bounding box
 * The width parameter is the width of the frame
 **/
 int check_width(int j, int length, int width){
   // how many pixels to move
   int move = 8;

   // determines how many times a ball can cross the image before reaching an edge
   int frameCount = j % (width/move);

   int x = frameCount * move;

  // if the bounding box reaches the edge, keep it there
  if(x + length >= width){
    x = width - length;
  }

   // ensure that the ball never goes out of bounds
  if((j/(width/move)) % 2 == 1){
    frameCount = -frameCount;
    x = width - x - length;
  }

   return x;
 }
