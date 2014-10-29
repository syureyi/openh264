#include <gtest/gtest.h>

#include <stdio.h>
#include "codec_def.h"
#include "codec_app_def.h"
#include "codec_api.h"
#include "wels_common_basis.h"
#include "mem_align.h"
#include "ls_defines.h"

using namespace WelsDec;
#define BUF_SIZE 100
//payload size exclude 6 bytes: 0001, nal type and final '\0'
#define PAYLOAD_SIZE (BUF_SIZE - 6)

class DecoderInterfaceTest : public ::testing::Test {
 public:
  virtual void SetUp() {
    int rv = WelsCreateDecoder (&m_pDec);
    ASSERT_EQ (0, rv);
    ASSERT_TRUE (m_pDec != NULL);
  }

  virtual void TearDown() {
    if (m_pDec) {
      WelsDestroyDecoder (m_pDec);
    }
  }
  //Init members
  void Init();
  //Uninit members
  void Uninit();
  //Read real bitstream from res 
 // void LoadBitstream(std::ifstream* file, BufferedData* buf);
  //Mock input data for test
  void MockPacketType (const EWelsNalUnitType eNalUnitType, const int iPacketLength);
  //Test Initialize/Uninitialize
  void TestInitUninit();
  //DECODER_OPTION_DATAFORMAT
  void TestDataFormat();
  //DECODER_OPTION_END_OF_STREAM
  void TestEndOfStream();
  //DECODER_OPTION_VCL_NAL
  void TestVclNal();
  //DECODER_OPTION_TEMPORAL_ID
  void TestTemporalId();
  //DECODER_OPTION_FRAME_NUM
  void TestFrameNum();
  //DECODER_OPTION_IDR_PIC_ID
  void TestIdrPicId();
  //DECODER_OPTION_LTR_MARKING_FLAG
  void TestLtrMarkingFlag();
  //DECODER_OPTION_LTR_MARKED_FRAME_NUM
  void TestLtrMarkedFrameNum();
  //DECODER_OPTION_ERROR_CON_IDC
  void TestErrorConIdc();
  //DECODER_OPTION_TRACE_LEVEL
  void TestTraceLevel();
  //DECODER_OPTION_TRACE_CALLBACK
  void TestTraceCallback();
  //DECODER_OPTION_TRACE_CALLBACK_CONTEXT
  void TestTraceCallbackContext();
  //Do whole tests here
  void DecoderInterfaceAll();
  void TestGetDecStatistics();


 public:
  ISVCDecoder* m_pDec;
  SDecodingParam m_sDecParam;
  SBufferInfo m_sBufferInfo;
  uint8_t* m_pData[3];
  unsigned char m_szBuffer[BUF_SIZE]; //for mocking packet
  int m_iBufLength; //record the valid data in m_szBuffer
};

//Init members
void DecoderInterfaceTest::Init() {
  memset (&m_sBufferInfo, 0, sizeof (SBufferInfo));
  memset (&m_sDecParam, 0, sizeof (SDecodingParam));
  m_sDecParam.pFileNameRestructed = NULL;
  m_sDecParam.eOutputColorFormat = (EVideoFormatType) (rand() % 100);
  m_sDecParam.uiCpuLoad = rand() % 100;
  m_sDecParam.uiTargetDqLayer = rand() % 100;
  m_sDecParam.eEcActiveIdc = (ERROR_CON_IDC) (rand() & 3);
  m_sDecParam.sVideoProperty.size = sizeof (SVideoProperty);
  m_sDecParam.sVideoProperty.eVideoBsType = (VIDEO_BITSTREAM_TYPE) (rand() % 3);

  m_pData[0] = m_pData[1] = m_pData[2] = NULL;
  m_szBuffer[0] = m_szBuffer[1] = m_szBuffer[2] = 0;
  m_szBuffer[3] = 1;
  m_iBufLength = 4;
  CM_RETURN eRet = (CM_RETURN) m_pDec->Initialize (&m_sDecParam);
  if ((m_sDecParam.eOutputColorFormat != videoFormatI420) &&
      (m_sDecParam.eOutputColorFormat != videoFormatInternal))
    ASSERT_EQ (eRet, cmUnsupportedData);
  else
    ASSERT_EQ (eRet, cmResultSuccess);
}

void DecoderInterfaceTest::Uninit() {
  if (m_pDec) {
    CM_RETURN eRet = (CM_RETURN) m_pDec->Uninitialize();
    ASSERT_EQ (eRet, cmResultSuccess);
  }
  memset (&m_sDecParam, 0, sizeof (SDecodingParam));
  memset (&m_sBufferInfo, 0, sizeof (SBufferInfo));
  m_pData[0] = m_pData[1] = m_pData[2] = NULL;
  m_iBufLength = 0;
}

void DecoderInterfaceTest::TestGetDecStatistics()
{
  uint8_t* pBuf = NULL;
  int32_t iBufPos = 0;
  int32_t iFileSize;
  int32_t i = 0;
  int32_t iSliceSize;
  int32_t iSliceIndex = 0;
  int32_t iEndOfStreamFlag = 0;
  int32_t iFrameCount=0;
  int32_t iError = 2;
  CM_RETURN eRet;
  SDecoderStatistics sDecStatic;
  Init();
  eRet=(CM_RETURN)m_pDec->SetOption(DECODER_OPTION_GET_STATISTICS,NULL);
  EXPECT_EQ (eRet, cmResultSuccess);
  m_pDec->SetOption(DECODER_OPTION_ERROR_CON_IDC,&iError);
   // EC error bs
  uint8_t uiStartCode[4] = {0, 0, 0, 1};
  FILE* pH264File	= fopen ("res/test.264", "rb");
  fseek (pH264File, 0L, SEEK_END);
  iFileSize = (int32_t) ftell (pH264File);
  fseek (pH264File, 0L, SEEK_SET);
  pBuf = new uint8_t[iFileSize + 4];
  fread (pBuf, 1, iFileSize, pH264File);
  memcpy (pBuf + iFileSize, &uiStartCode[0], 4); //confirmed_safe_unsafe_usage
  while (true) {
  if (iBufPos >= iFileSize) {
      iEndOfStreamFlag = true;
      if (iEndOfStreamFlag)
        m_pDec->SetOption (DECODER_OPTION_END_OF_STREAM, (void*)&iEndOfStreamFlag);
      break;
    }
    for (i = 0; i < iFileSize; i++) {
      if ((pBuf[iBufPos + i] == 0 && pBuf[iBufPos + i + 1] == 0 && pBuf[iBufPos + i + 2] == 0 && pBuf[iBufPos + i + 3] == 1
           && i > 0)) {
        break;
      }
    }
  iSliceSize = i;	
  m_pDec->DecodeFrame2 (pBuf + iBufPos, iSliceSize, m_pData, &m_sBufferInfo);
  if(m_sBufferInfo.iBufferStatus ==1)
  {
	iFrameCount++;
	switch(iFrameCount)
		{
		case 1:
			m_pDec->GetOption(DECODER_OPTION_GET_STATISTICS,&sDecStatic);
			EXPECT_TRUE(sDecStatic.bErrorConcealed);
			EXPECT_EQ(66,sDecStatic.uiAvgEcRatio);
			EXPECT_EQ(1,sDecStatic.uiDecodedFrameCount);
			EXPECT_EQ(1,sDecStatic.uiFrameRecvNum);
			EXPECT_EQ(288,sDecStatic.uiHeight);
			EXPECT_EQ(1,sDecStatic.uiIDRRecvNum);
			EXPECT_EQ(1,sDecStatic.uiResolutionChangeTimes);
			EXPECT_EQ(352,sDecStatic.uiWidth);
			break;
		case 2:
			iError = 1;
			m_pDec->SetOption(DECODER_OPTION_ERROR_CON_IDC,&iError);
			m_pDec->GetOption(DECODER_OPTION_GET_STATISTICS,&sDecStatic);
			EXPECT_TRUE(sDecStatic.bErrorConcealed);
			EXPECT_LT(0,sDecStatic.fAverageFrameSpeedInMs);
			EXPECT_EQ(66,sDecStatic.uiAvgEcRatio);
			EXPECT_EQ(1,sDecStatic.uiDecodedFrameCount);
			EXPECT_EQ(1,sDecStatic.uiFrameRecvNum);
			EXPECT_EQ(288,sDecStatic.uiHeight);
			EXPECT_EQ(0,sDecStatic.uiIDRRecvNum);
			EXPECT_EQ(1,sDecStatic.uiResolutionChangeTimes);
			EXPECT_EQ(352,sDecStatic.uiWidth);
			break;
		case 3:
			m_pDec->GetOption(DECODER_OPTION_GET_STATISTICS,&sDecStatic);
			EXPECT_FALSE(sDecStatic.bErrorConcealed);
			EXPECT_GT(1,sDecStatic.fAverageFrameSpeedInMs);
			EXPECT_EQ(0,sDecStatic.uiAvgEcRatio);
			EXPECT_EQ(1,sDecStatic.uiDecodedFrameCount);
			EXPECT_EQ(1,sDecStatic.uiFrameRecvNum);
			EXPECT_EQ(480,sDecStatic.uiHeight);
			EXPECT_EQ(1,sDecStatic.uiIDRRecvNum);
			EXPECT_EQ(1,sDecStatic.uiResolutionChangeTimes);
			EXPECT_EQ(640,sDecStatic.uiWidth);
			break;
		case 4:
			break;
		case 5:
			m_pDec->GetOption(DECODER_OPTION_GET_STATISTICS,&sDecStatic);
			EXPECT_TRUE(sDecStatic.bErrorConcealed);
			EXPECT_EQ(100,sDecStatic.uiAvgEcRatio);
			EXPECT_EQ(2,sDecStatic.uiDecodedFrameCount);
			EXPECT_EQ(2,sDecStatic.uiFrameRecvNum);
			EXPECT_EQ(288,sDecStatic.uiHeight);
			EXPECT_EQ(1,sDecStatic.uiIDRRecvNum);
			EXPECT_EQ(2,sDecStatic.uiResolutionChangeTimes);
			EXPECT_EQ(352,sDecStatic.uiWidth);
			

			
			m_pDec->GetOption(DECODER_OPTION_GET_STATISTICS,&sDecStatic);
			EXPECT_FALSE(sDecStatic.bErrorConcealed);
			EXPECT_EQ(0,sDecStatic.uiAvgEcRatio);
           EXPECT_EQ(0,sDecStatic.uiDecodedFrameCount);
   EXPECT_EQ(0,sDecStatic.uiFrameRecvNum);
   EXPECT_EQ(0,sDecStatic.uiHeight);
   EXPECT_EQ(0,sDecStatic.uiIDRRecvNum);
   EXPECT_EQ(0,sDecStatic.uiResolutionChangeTimes);
   EXPECT_EQ(0,sDecStatic.uiWidth);
			break;
		default:
			break;


		}
		
	}
  iBufPos += iSliceSize;
  ++ iSliceIndex;
   }
   fclose(pH264File);
  


   Uninit();
  
}
//Mock input data for test
void DecoderInterfaceTest::MockPacketType (const EWelsNalUnitType eNalUnitType, const int iPacketLength) {
  switch (eNalUnitType) {
  case NAL_UNIT_SEI:
    m_szBuffer[m_iBufLength++] = 6;
    break;
  case NAL_UNIT_SPS:
    m_szBuffer[m_iBufLength++] = 67;
    break;
  case NAL_UNIT_PPS:
    m_szBuffer[m_iBufLength++] = 68;
    break;
  case NAL_UNIT_SUBSET_SPS:
    m_szBuffer[m_iBufLength++] = 15;
    break;
  case NAL_UNIT_PREFIX:
    m_szBuffer[m_iBufLength++] = 14;
    break;
  case NAL_UNIT_CODED_SLICE:
    m_szBuffer[m_iBufLength++] = 61;
    break;
  case NAL_UNIT_CODED_SLICE_IDR:
    m_szBuffer[m_iBufLength++] = 65;
    break;
  default:
    m_szBuffer[m_iBufLength++] = 0; //NAL_UNIT_UNSPEC_0
    break;

    int iAddLength = iPacketLength - 5; //excluding 0001 and type
    if (iAddLength > PAYLOAD_SIZE)
      iAddLength = PAYLOAD_SIZE;
    for (int i = 0; i < iAddLength; ++i) {
      m_szBuffer[m_iBufLength++] = rand() % 256;
    }
    m_szBuffer[m_iBufLength++] = '\0';

  }
}

//Test Initialize/Uninitialize
void DecoderInterfaceTest::TestInitUninit() {
  int iOutput;
  CM_RETURN eRet;
  //No initialize, no GetOption can be done
  m_pDec->Uninitialize();
  for (int i = 0; i <= (int) DECODER_OPTION_TRACE_CALLBACK_CONTEXT; ++i) {
    eRet = (CM_RETURN) m_pDec->GetOption ((DECODER_OPTION) i, &iOutput);
    EXPECT_EQ (eRet, cmInitExpected);
  }
  //Initialize first, can get input color format
  m_sDecParam.eOutputColorFormat = (EVideoFormatType) 20; //just for test
  m_pDec->Initialize (&m_sDecParam);
  eRet = (CM_RETURN) m_pDec->GetOption (DECODER_OPTION_DATAFORMAT, &iOutput);
  EXPECT_EQ (eRet, cmResultSuccess);
  EXPECT_EQ ((int32_t) videoFormatI420, iOutput);

  //Uninitialize, no GetOption can be done
  m_pDec->Uninitialize();
  iOutput = 21;
  for (int i = 0; i <= (int) DECODER_OPTION_TRACE_CALLBACK_CONTEXT; ++i) {
    eRet = (CM_RETURN) m_pDec->GetOption ((DECODER_OPTION) i, &iOutput);
    EXPECT_EQ (iOutput, 21);
    EXPECT_EQ (eRet, cmInitExpected);
  }
}

//DECODER_OPTION_DATAFORMAT
void DecoderInterfaceTest::TestDataFormat() {
  int iTmp = rand();
  int iOut;
  CM_RETURN eRet;

  Init();

  //invalid input
  eRet = (CM_RETURN) m_pDec->SetOption (DECODER_OPTION_DATAFORMAT, NULL);
  EXPECT_EQ (eRet, cmInitParaError);

  //valid input
  eRet = (CM_RETURN) m_pDec->SetOption (DECODER_OPTION_DATAFORMAT, &iTmp);
  if ((iTmp != (int32_t) videoFormatI420) && (iTmp != (int32_t) videoFormatInternal))
    EXPECT_EQ (eRet, cmUnsupportedData);
  else
    EXPECT_EQ (eRet, cmResultSuccess);
  eRet = (CM_RETURN) m_pDec->GetOption (DECODER_OPTION_DATAFORMAT, &iOut);
  EXPECT_EQ (eRet, cmResultSuccess);

  EXPECT_EQ (iOut, (int32_t) videoFormatI420);

  Uninit();
}

//DECODER_OPTION_END_OF_STREAM
void DecoderInterfaceTest::TestEndOfStream() {
  int iTmp, iOut;
  CM_RETURN eRet;

  Init();

  //invalid input
  eRet = (CM_RETURN) m_pDec->SetOption (DECODER_OPTION_END_OF_STREAM, NULL);
  EXPECT_EQ (eRet, cmInitParaError);

  //valid random input
  for (int i = 0; i < 10; ++i) {
    iTmp = rand();
    eRet = (CM_RETURN) m_pDec->SetOption (DECODER_OPTION_END_OF_STREAM, &iTmp);
    EXPECT_EQ (eRet, cmResultSuccess);
    eRet = (CM_RETURN) m_pDec->GetOption (DECODER_OPTION_END_OF_STREAM, &iOut);
    EXPECT_EQ (eRet, cmResultSuccess);
    EXPECT_EQ (iOut, iTmp != 0);
  }

  //set false as input
  iTmp = false;
  eRet = (CM_RETURN) m_pDec->SetOption (DECODER_OPTION_END_OF_STREAM, &iTmp);
  EXPECT_EQ (eRet, cmResultSuccess);
  eRet = (CM_RETURN) m_pDec->GetOption (DECODER_OPTION_END_OF_STREAM, &iOut);
  EXPECT_EQ (eRet, cmResultSuccess);

  EXPECT_EQ (iOut, false);

  //set true as input
  iTmp = true;
  eRet = (CM_RETURN) m_pDec->SetOption (DECODER_OPTION_END_OF_STREAM, &iTmp);
  EXPECT_EQ (eRet, cmResultSuccess);
  eRet = (CM_RETURN) m_pDec->GetOption (DECODER_OPTION_END_OF_STREAM, &iOut);
  EXPECT_EQ (eRet, cmResultSuccess);

  EXPECT_EQ (iOut, true);

  //Mock data packet in
  //Test NULL data input for decoder, should be true for EOS
  eRet = (CM_RETURN) m_pDec->DecodeFrame2 (NULL, 0, m_pData, &m_sBufferInfo);
  EXPECT_EQ (eRet, 0); //decode should return OK
  eRet = (CM_RETURN) m_pDec->GetOption (DECODER_OPTION_END_OF_STREAM, &iOut);
  EXPECT_EQ (iOut, true); //decoder should have EOS == true

  //Test valid data input for decoder, should be false for EOS
  MockPacketType (NAL_UNIT_UNSPEC_0, 50);
  eRet = (CM_RETURN) m_pDec->DecodeFrame2 (m_szBuffer, m_iBufLength, m_pData, &m_sBufferInfo);
  eRet = (CM_RETURN) m_pDec->GetOption (DECODER_OPTION_END_OF_STREAM, &iOut);
  EXPECT_EQ (iOut, false); //decoder should have EOS == false
  //Test NULL data input for decoder, should be true for EOS
  eRet = (CM_RETURN) m_pDec->DecodeFrame2 (NULL, 0, m_pData, &m_sBufferInfo);
  eRet = (CM_RETURN) m_pDec->GetOption (DECODER_OPTION_END_OF_STREAM, &iOut);
  EXPECT_EQ (iOut, true); //decoder should have EOS == true

  Uninit();
}


//DECODER_OPTION_VCL_NAL
//Here Test illegal bitstream input
//legal bitstream decoding test, please see api test
void DecoderInterfaceTest::TestVclNal() {
  int iTmp, iOut;
  CM_RETURN eRet;

  Init();

  //Test SetOption
  //VclNal never supports SetOption
  iTmp = rand();
  eRet = (CM_RETURN) m_pDec->SetOption (DECODER_OPTION_VCL_NAL, &iTmp);
  EXPECT_EQ (eRet, cmInitParaError);

  //Test GetOption
  //invalid input
  eRet = (CM_RETURN) m_pDec->GetOption (DECODER_OPTION_VCL_NAL, NULL);
  EXPECT_EQ (eRet, cmInitParaError);

  //valid input without actual decoding
  eRet = (CM_RETURN) m_pDec->GetOption (DECODER_OPTION_VCL_NAL, &iOut);
  EXPECT_EQ (eRet, cmResultSuccess);
  EXPECT_EQ (iOut, FEEDBACK_NON_VCL_NAL);

  //valid input with decoding error
  MockPacketType (NAL_UNIT_CODED_SLICE_IDR, 50);
  m_pDec->DecodeFrame2 (m_szBuffer, m_iBufLength, m_pData, &m_sBufferInfo);
  eRet = (CM_RETURN) m_pDec->GetOption (DECODER_OPTION_VCL_NAL, &iOut);
  EXPECT_EQ (eRet, cmResultSuccess);
  EXPECT_EQ (iOut, FEEDBACK_UNKNOWN_NAL);
  m_pDec->DecodeFrame2 (NULL, 0, m_pData, &m_sBufferInfo);
  eRet = (CM_RETURN) m_pDec->GetOption (DECODER_OPTION_VCL_NAL, &iOut);
  EXPECT_EQ (eRet, cmResultSuccess);
  EXPECT_EQ (iOut, FEEDBACK_UNKNOWN_NAL);

  Uninit();
}

//DECODER_OPTION_TEMPORAL_ID
void DecoderInterfaceTest::TestTemporalId() {
  //TODO
}

//DECODER_OPTION_FRAME_NUM
void DecoderInterfaceTest::TestFrameNum() {
  //TODO
}

//DECODER_OPTION_IDR_PIC_ID
void DecoderInterfaceTest::TestIdrPicId() {
  //TODO
}

//DECODER_OPTION_LTR_MARKING_FLAG
void DecoderInterfaceTest::TestLtrMarkingFlag() {
  //TODO
}

//DECODER_OPTION_LTR_MARKED_FRAME_NUM
void DecoderInterfaceTest::TestLtrMarkedFrameNum() {
  //TODO
}

//DECODER_OPTION_ERROR_CON_IDC
void DecoderInterfaceTest::TestErrorConIdc() {
  //TODO
}

//DECODER_OPTION_TRACE_LEVEL
void DecoderInterfaceTest::TestTraceLevel() {
  //TODO
}

//DECODER_OPTION_TRACE_CALLBACK
void DecoderInterfaceTest::TestTraceCallback() {
  //TODO
}

//DECODER_OPTION_TRACE_CALLBACK_CONTEXT
void DecoderInterfaceTest::TestTraceCallbackContext() {
  //TODO
}


//TEST here for whole tests
TEST_F (DecoderInterfaceTest, DecoderInterfaceAll) {

  //Initialize Uninitialize
  TestInitUninit();
  //DECODER_OPTION_DATAFORMAT
  TestDataFormat();
  //DECODER_OPTION_END_OF_STREAM
  TestEndOfStream();
  //DECODER_OPTION_VCL_NAL
  TestVclNal();
  //DECODER_OPTION_TEMPORAL_ID
  TestTemporalId();
  //DECODER_OPTION_FRAME_NUM
  TestFrameNum();
  //DECODER_OPTION_IDR_PIC_ID
  TestIdrPicId();
  //DECODER_OPTION_LTR_MARKING_FLAG
  TestLtrMarkingFlag();
  //DECODER_OPTION_LTR_MARKED_FRAME_NUM
  TestLtrMarkedFrameNum();
  //DECODER_OPTION_ERROR_CON_IDC
  TestErrorConIdc();
  //DECODER_OPTION_TRACE_LEVEL
  TestTraceLevel();
  //DECODER_OPTION_TRACE_CALLBACK
  TestTraceCallback();
  //DECODER_OPTION_TRACE_CALLBACK_CONTEXT
  TestTraceCallbackContext();
  TestGetDecStatistics();
}


