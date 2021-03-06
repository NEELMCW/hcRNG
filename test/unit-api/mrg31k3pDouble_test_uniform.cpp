#include <hcRNG/mrg31k3p.h>
#include <hcRNG/hcRNG.h>
#include <hc.hpp>
#include <hc_am.hpp>
#include "gtest/gtest.h"
using namespace hc;

void multistream_fill_array_uniform(size_t spwi, size_t gsize, size_t quota, int substream_length, hcrngMrg31k3pStream* streams, double* out_)
{
  for (size_t i = 0; i < quota; i++) {
      for (size_t gid = 0; gid < gsize; gid++) {
          hcrngMrg31k3pStream* s = &streams[spwi * gid];
          double* out = &out_[spwi * (i * gsize + gid)];
          if ((i > 0) && (substream_length > 0) && (i % substream_length == 0))
              hcrngMrg31k3pForwardToNextSubstreams(spwi, s);
          else if ((i > 0) && (substream_length < 0) && (i % (-substream_length) == 0))
              hcrngMrg31k3pRewindSubstreams(spwi, s);
          for (size_t sid = 0; sid < spwi; sid++) {
              out[sid] = hcrngMrg31k3pRandomU01(&s[sid]);
          }
      }
  }
}



TEST(mrg31k3pDouble_test_uniform, Functional_check_mrg31k3pDouble_uniform)
{
        hcrngMrg31k3pStream* stream = NULL;
        hcrngStatus status = HCRNG_SUCCESS;
        bool ispassed1 = 1, ispassed2 = 1;
        size_t streamBufferSize;
        size_t NbrStreams = 1;
        size_t streamCount = 10;
        size_t numberCount = 100;
        int stream_length = 5;
        size_t streams_per_thread = 2;
        double *Random1 = (double*) malloc(sizeof(double) * numberCount);
        double *Random2 = (double*) malloc(sizeof(double) * numberCount);
	std::vector<hc::accelerator>acc = hc::accelerator::get_all();
        accelerator_view accl_view = (acc[1].get_default_view());
        double *outBufferDevice = hc::am_alloc(sizeof(double) * numberCount, acc[1], 0);
        hcrngMrg31k3pStream *streams = hcrngMrg31k3pCreateStreams(NULL, streamCount, &streamBufferSize, NULL);
        hcrngMrg31k3pStream *streams_buffer = hc::am_alloc(sizeof(hcrngMrg31k3pStream) * streamCount, acc[1], 0);
        accl_view.copy(streams, streams_buffer, streamCount* sizeof(hcrngMrg31k3pStream));
        status = hcrngMrg31k3pDeviceRandomU01Array_double(accl_view, streamCount, streams_buffer, numberCount, outBufferDevice);
        EXPECT_EQ(status, 0);
        accl_view.copy(outBufferDevice, Random1, numberCount * sizeof(double));
        for (size_t i = 0; i < numberCount; i++)
           Random2[i] = hcrngMrg31k3pRandomU01(&streams[i % streamCount]);   
        for(int i =0; i < numberCount; i++) {
           EXPECT_EQ(Random1[i], Random2[i]);
        }
        double *Random3 = (double*) malloc(sizeof(double) * numberCount);
        double *Random4 = (double*) malloc(sizeof(double) * numberCount);
        double *outBufferDevice_substream = hc::am_alloc(sizeof(double) * numberCount, acc[1], 0);
        status = hcrngMrg31k3pDeviceRandomU01Array_double(accl_view, streamCount, streams_buffer, numberCount, outBufferDevice_substream, stream_length, streams_per_thread);
        EXPECT_EQ(status, 0);
        accl_view.copy(outBufferDevice_substream, Random3, numberCount * sizeof(double));
        multistream_fill_array_uniform(streams_per_thread, streamCount/streams_per_thread, numberCount/streamCount, stream_length, streams, Random4);
        for(int i =0; i < numberCount; i++) {
           EXPECT_EQ(Random3[i], Random4[i]);
        }

}


