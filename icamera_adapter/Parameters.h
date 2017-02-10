/*
 * Copyright (C) 2015-2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
// this file is from libcamhal and ideally should be shared with it somehow
/*
 *
 * Filename: Parameters.h
 *
 * ------------------------------------------------------------------------------
 * REVISION HISTORY
 *     Version        0.1        Initialize camera parameters API
 *     Version        0.2        Merge all the types to this file
 *     Version        0.3        Add AE compensation related APIs.
 *     Version        0.31       Add manual color matrix APIs.
 *     Version        0.32       Add manual AE/AWB converge speed APIs.
 *     Version        0.33       Add timestamp variable in camera_buffer_t
 *     Version        0.34       Add AE window weight grid API
 *     Version        0.40       Add Data Structure for HAL 3.3
 *     Version        0.41       Add API getSupportedAeExposureTimeRange
 *                               Add API getSupportedAeGainRange
 *     Version        0.42       Add API updateDebugLevel
 *     Version        0.43       Add API set and get deinterlace mode
 *
 * ------------------------------------------------------------------------------
 */

#ifndef _CAMERA_PARAMETERS_H_
#define _CAMERA_PARAMETERS_H_

#include "camera/CameraMetadata.h"

#include <vector>
#include <stdint.h>

namespace icamera {
using android::CameraMetadata;

class RWLock;

/***************Start of Camera Basic Data Structure ****************************/
/**
 * Basic definition will be inherited by more complicated structure.
 * MUST be all "int" in this structure.
 */
typedef struct {
    int width;
    int height;
} camera_resolution_t;
typedef std::vector<camera_resolution_t> camera_res_array_t;

/**
 * \struct stream_t: stream basic info
 *
 * \note
 *   The first four fields are read from the XML. Please keep the order of them.
 */
#define STREAM_MEMBER_NUM   (4) //number of valid members from profiles in the stream_t
typedef struct {
/*Follow four fields must be in strict order */
    int format;    /**< stream format refer to v4l2 definition https://linuxtv.org/downloads/v4l-dvb-apis/pixfmt.html */
    int width;     /**< image width */
    int height;    /**< image height */
    int field;     /**< refer to v4l2 definition https://linuxtv.org/downloads/v4l-dvb-apis/field-order.html#v4l2-field */

/*
* The buffer geometry introduction.
* The YUV image is formed with Y:Luma and UV:Chroma. And there are
* two kinds of styles for YUV format: planar and packed.
*
*   YUV420:NV12
*
*            YUV420(720x480) sampling
*
*       |<----width+padding=alignedBpl----->|
*     Y *-------*-------*-------*-------*....-----
*       |                               |   :  ^
*       |   # UV            #           |   :  |
*       |                               |   :  |
*       *-------*-------*-------*-------*....  |
*       |                               |   :  |
*       |   #               #           |   :  |
*       |                               |   :  |
*       *-------*-------*-------*-------*.... (height * 3 / 2)
*       |                               |   :  |
*       |   #               #           |   :  |
*       |                               |   :  |
*       *-------*-------*-------*-------*....  |
*       |                               |   :  |
*       |   #               #           |   :  |
*       |                               |   :  v
*       *-------*-------*-------*-------*....-----
*
*         The data stored in memory
*          ____________w___________ .....
*         |Y0|Y1                   |    :
*         |                        |    :
*         h                        h    :
*         |                        |    :
*         |                        |    :
*         |________________________|....:
*         |U|V|U|V                 |    :
*        h/2                      h/2   :
*         |____________w___________|....:
*
*       bpp = 12
*       bpl = width*(bpp/8) = width;
*       stride = align64(bpl)/(bpp/8):
*
*   YUV422:YUY2
*
*           YUV422(720x480) sampling
*
*       |<--(width*2)+padding=alignedBpl-->|
*   YUV *#----*#-----*#-----*#-----*#....-----
*       *#----*#-----*#-----*#-----*#....  |
*       *#----*#-----*#-----*#-----*#....  |
*       *#----*#-----*#-----*#-----*#....  |
*       *#----*#-----*#-----*#-----*#.... (height)
*       *#----*#-----*#-----*#-----*#....  |
*       *#----*#-----*#-----*#-----*#....  |
*       *#----*#-----*#-----*#-----*#....  |
*       *#----*#-----*#-----*#-----*#....  |
*       *#----*#-----*#-----*#-----*#....-----
*
*         The data stored in memory
*          ____________w___________ .....
*         |Y0|Cb|Y1|Cr             |    :
*         |                        |    :
*         |                        |    :
*         |                        |    :
*         h                        h    :
*         |                        |    :
*         |                        |    :
*         |                        |    :
*         |____________w___________|....:
*
*       bpp = 16
*       bpl = width*(bpp/8) = width * 2;
*       stride = align64(bpl)/(bpp/8):
*
*       Note: The stride defined in HAL is different with byte per line.
*/
    int stride;    /**< stride = width + padding */
    int size;      /**< real buffer size */

    int id;        /**< Id that is filled by HAL. */
    int memType;   /**< buffer memory type filled by app, refer to https://linuxtv.org/downloads/v4l-dvb-apis/io.html */

    /**
     * The maximum number of buffers the HAL device may need to have dequeued at
     * the same time. The HAL device may not have more buffers in-flight from
     * this stream than this value.
     */
    uint32_t max_buffers;

    /**
     * A handle to HAL-private information for the stream. Will not be inspected
     * by the framework code.
     */
    void *priv;

    /**
     * Output rotation of the stream
     */
    int rotation;

    /* reserved for future use */
    void *reserved[7];
} stream_t;

typedef std::vector<stream_t> stream_array_t;

/**
 * \struct stream_config_t: stream configuration info
 *
 * Contains all streams info in this configuration.
 */
typedef struct {
    int num_streams; /**< number of streams in this configuration */
    stream_t    *streams; /**< streams list */
    /**
     * >= CAMERA_DEVICE_API_VERSION_3_3:
     *
     * The operation mode of streams in this configuration, one of the value defined in
     * camera3_stream_configuration_mode_t.
     * The HAL can use this mode as an indicator to set the stream property (e.g.,
     * camera3_stream->max_buffers) appropriately. For example, if the configuration is
     * CAMERA3_STREAM_CONFIGURATION_CONSTRAINED_HIGH_SPEED_MODE, the HAL may want to set aside more
     * buffers for batch mode operation (see android.control.availableHighSpeedVideoConfigurations
     * for batch mode definition).
     *
     */
    uint32_t operation_mode;
} stream_config_t;

typedef enum {
    BUFFER_FLAG_DMA_EXPORT = 1<<0,
    BUFFER_FLAG_INTERNAL = 1<<1,
    BUFFER_FLAG_SW_READ = 1<<2,
    BUFFER_FLAG_SW_WRITE = 1<<3,
} camera_buffer_flags_t;

/**
 * \struct camera_buffer_t: camera buffer info
 *
 * camera buffer is used to carry device frames. Application allocate buffer structure,
 * according to memory type to allocate memory and queue to device.
 */
typedef struct {
    stream_t s;   /**< stream info */
    void *addr;   /**< buffer addr for userptr and mmap memory mode */
    int index;    /**< buffer index, filled by HAL. it is used for qbuf and dqbuf in order */
    int sequence; /**< buffer sequence, filled by HAL, to record buffer dqueue sequence from device */
    int dmafd;    /**< buffer dmafd for DMA import and export mode */
    int flags;    /**< buffer flags, used to specify buffer properties */
    uint64_t timestamp; /**< buffer timestamp, it's a time reference measured in nanosecond */
    int reserved; /**< reserved for future */
} camera_buffer_t;

/***************End of Camera Basic Data Structure ****************************/


/*******************Start of Camera Parameters Definition**********************/
/**
 * \enum camera_features: camera supportted features.
 */
typedef enum {
    MANUAL_EXPOSURE,       /**< Allow user to controll exposure time and ISO manually */
    MANUAL_WHITE_BALANCE,  /**< Allow user to controll AWB mode, cct range, and gain */
    IMAGE_ENHANCEMENT,     /**< Sharpness, Brightness, Contrast, Hue, Saturation */
    NOISE_REDUCTION,       /**< Allow user to control NR mode and NR level */
    SCENE_MODE,            /**< Allow user to control scene mode */
    WEIGHT_GRID_MODE,      /**< Allow user to control custom weight grid mode */
    PER_FRAME_CONTROL,     /**< Allow user to control most of parameters for each frame */
    INVALID_FEATURE
} camera_features;
typedef std::vector<camera_features> camera_features_list_t;

typedef enum {
    AE_MODE_AUTO,
    AE_MODE_MANUAL
} camera_ae_mode_t;

typedef enum {
    ANTIBANDING_MODE_AUTO,
    ANTIBANDING_MODE_50HZ,
    ANTIBANDING_MODE_60HZ,
    ANTIBANDING_MODE_OFF,
} camera_antibanding_mode_t;

typedef enum {
    SCENE_MODE_AUTO,
    SCENE_MODE_HDR,
    SCENE_MODE_ULL,
    SCENE_MODE_INDOOR,
    SCENE_MODE_OUTDOOR,
    SCENE_MODE_DISABLED,
    SCENE_MODE_MAX
} camera_scene_mode_t;

typedef struct {
    camera_scene_mode_t scene_mode;
    int exposure_time_min; // The supported minimum exposure time, and the unit is us.
    int exposure_time_max; // The supported maximum exposure time, and the unit is us.
} camera_ae_exposure_time_range_t;

typedef struct {
    camera_scene_mode_t scene_mode;
    float gain_min; // The supported minimum sensor gain, and the unit is db.
    float gain_max; // The supported minimum sensor gain, and the unit is db.
} camera_ae_gain_range_t;

typedef enum {
    WEIGHT_GRID_AUTO,
    CUSTOM_WEIGHT_GRID_1,
    CUSTOM_WEIGHT_GRID_2,
    CUSTOM_WEIGHT_GRID_3
} camera_weight_grid_mode_t;

typedef enum {
    AWB_MODE_AUTO,
    AWB_MODE_INCANDESCENT,
    AWB_MODE_FLUORESCENT,
    AWB_MODE_DAYLIGHT,
    AWB_MODE_FULL_OVERCAST,
    AWB_MODE_PARTLY_OVERCAST,
    AWB_MODE_SUNSET,
    AWB_MODE_VIDEO_CONFERENCE,
    AWB_MODE_MANUAL_CCT_RANGE,
    AWB_MODE_MANUAL_WHITE_POINT,
    AWB_MODE_MANUAL_GAIN,
    AWB_MODE_MANUAL_COLOR_TRANSFORM,
} camera_awb_mode_t;

typedef enum
{
    CAM_EFFECT_NONE = 0,
    CAM_EFFECT_MONO,
    CAM_EFFECT_SEPIA,
    CAM_EFFECT_NEGATIVE,
    CAM_EFFECT_SKY_BLUE,
    CAM_EFFECT_GRASS_GREEN,
    CAM_EFFECT_SKIN_WHITEN_LOW,
    CAM_EFFECT_SKIN_WHITEN,
    CAM_EFFECT_SKIN_WHITEN_HIGH,
    CAM_EFFECT_VIVID,
} camera_effect_mode_t;

typedef struct {
    int r_gain;
    int g_gain;
    int b_gain;
} camera_awb_gains_t;

typedef struct {
    float color_transform[3][3];
} camera_color_transform_t;

typedef enum {
    NR_MODE_OFF,
    NR_MODE_AUTO,
    NR_MODE_MANUAL_NORMAL,
    NR_MODE_MANUAL_EXPERT,
} camera_nr_mode_t;

typedef struct {
    int overall;
    int spatial;
    int temporal;
} camera_nr_level_t;

typedef enum {
    IRIS_MODE_AUTO,
    IRIS_MODE_MANUAL,
    IRIS_MODE_CUSTOMIZED,
} camera_iris_mode_t;

typedef enum {
    WDR_MODE_AUTO,
    WDR_MODE_ON,
    WDR_MODE_OFF,
} camera_wdr_mode_t;

typedef enum {
    BLC_AREA_MODE_OFF,
    BLC_AREA_MODE_ON,
} camera_blc_area_mode_t;

typedef struct {
    int left;
    int top;
    int right;
    int bottom;
    int weight;
} camera_window_t;
typedef std::vector<camera_window_t> camera_window_list_t;

typedef struct {
    int sharpness;
    int brightness;
    int contrast;
    int hue;
    int saturation;
} camera_image_enhancement_t;

typedef struct {
    int x;
    int y;
} camera_coordinate_t;

typedef struct
{
    int left;
    int top;
    int right;
    int bottom;
} camera_coordinate_system_t;

typedef struct {
    int min;
    int max;
} camera_range_t;
typedef std::vector<camera_range_t> camera_range_array_t;

typedef struct {
    int numerator;
    int denominator;
} camera_rational_t;

typedef enum {
    CONVERGE_NORMAL,
    CONVERGE_MID,
    CONVERGE_LOW,
    CONVERGE_MAX
} camera_converge_speed_t;

typedef enum {
    CONVERGE_SPEED_MODE_AIQ,
    CONVERGE_SPEED_MODE_HAL
} camera_converge_speed_mode_t;

typedef enum {
    DISTRIBUTION_AUTO,
    DISTRIBUTION_SHUTTER,
    DISTRIBUTION_ISO,
    DISTRIBUTION_APERTURE
} camera_ae_distribution_priority_t;

typedef enum {
    DEINTERLACE_OFF,
    DEINTERLACE_WEAVING
} camera_deinterlace_mode_t;

typedef enum {
    VIDEO_STABILIZATION_MODE_OFF,
    VIDEO_STABILIZATION_MODE_ON
} camera_video_stabilization_mode_t;

typedef enum {
    CAMERA_FULL_MODE_YUV_COLOR_RANGE,
    CAMERA_REDUCED_MODE_YUV_COLOR_RANGE
} camera_yuv_color_range_mode_t;

/**
 * \class Parameters
 *
 * \brief
 *   Manage parameter's data structure, and provide set and get parameters
 *
 * This class provides a thread safe management to internal parameter's data
 * structure, and helps client to easily set parameters to and get parameters
 * from camera device.
 *
 * \version 0.1
 *
 */
class Parameters {
public:
    Parameters();
    Parameters(const Parameters& other);
    Parameters& operator=(const Parameters& other);
    ~Parameters();
    /**
     * \brief Merge and update current parameter with other
     *
     * \param[in] Parameters other: parameter source
     *
     * \return void
     */
    void merge(const Parameters& other);

    /**
     * \brief Merge and update current parameter with metadata
     *
     * \param[in] CameraMetadata metadata: internal parameter data structure source
     *
     * This means to be used internally since CameraMetadata is not published
     * to outside.
     *
     * \return void
     */
    void merge(const CameraMetadata* metadata);

public:
    /**
     * \brief Set exposure mode(auto/manual).
     *
     * "auto" means 3a algorithm will control exposure time and gain automatically.
     * "manual" means user can control exposure time or gain, or both of them.
     * Under manual mode, if user only set one of exposure time or gain, then 3a algorithm
     * will help to calculate the other one.
     *
     * \param[in] camera_ae_mode_t aeMode
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int setAeMode(camera_ae_mode_t aeMode);

    /**
     * \brief Get exposure mode
     *
     * \param[out] aeMode: Currently used ae mode will be set to aeMode if 0 is returned.
     *
     * \return 0 if exposure mode was set, otherwise non-0 value is returned.
     */
    int getAeMode(camera_ae_mode_t& aeMode) const;

    int setAeRegions(camera_window_list_t aeRegions);
    int getAeRegions(camera_window_list_t& aeRegions) const;

    /**
     * \brief Set exposure time whose unit is microsecond(us).
     *
     * The exposure time only take effect when ae mode set to manual.
     *
     * \param[in] int64_t exposureTime
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int setExposureTime(int64_t exposureTime);

    /**
     * \brief Get exposure time whose unit is microsecond(us).
     *
     * \param[out] int64_t& exposureTime: exposure time if be set in exposureTime if 0 is returned.
     *
     * \return 0 if exposure time was set, non-0 means no exposure time was set.
     */
    int getExposureTime(int64_t& exposureTime) const;

    /**
     * \brief Set sensor gain whose unit is db.
     * The sensor gain only take effect when ae mode set to manual.
     *
     * \param[in] float gain
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int setSensitivityGain(float gain);

    /**
     * \brief Get sensor gain whose unit is db.
     *
     * \param[out] float gain
     *
     * \return 0 if sensor gain was set, non-0 means no sensor gain was set.
     */
    int getSensitivityGain(float& gain) const;

    /**
     * \brief Set ae compensation whose unit is compensation step.
     *
     * The adjustment is measured as a count of steps, with the
     * step size defined ae compensation step and the allowed range by ae compensation range.
     *
     * For example, if the exposure value (EV) step is 0.333, '6'
     * will mean an exposure compensation of +2 EV; -3 will mean an
     * exposure compensation of -1 EV. One EV represents a doubling of image brightness.
     *
     * In the event of exposure compensation value being changed, camera device
     * may take several frames to reach the newly requested exposure target.
     *
     * \param[in] int ev
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int setAeCompensation(int ev);

    /**
     * \brief Get ae compensation whose unit is compensation step.
     *
     * \param[out] int ev
     *
     * \return 0 if ae compensation was set, non-0 means no ae compensation was set.
     */
    int getAeCompensation(int& ev) const;

    int setFrameRate(int fps);
    int getFrameRate(int& fps) const;

    int setAntiBandingMode(camera_antibanding_mode_t bandingMode);
    int getAntiBandingMode(camera_antibanding_mode_t& bandingMode) const;

    /**
     * \brief Set white balance mode
     *
     * White balance mode could be one of totally auto, preset cct range, customized cct range, customized
     * white area, customize gains.
     *
     * \param[in] camera_awb_mode_t awbMode
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int setAwbMode(camera_awb_mode_t awbMode);

    /**
     * \brief Get white balance mode currently used.
     *
     * \param[out] camera_awb_mode_t& awbMode
     *
     * \return 0 if awb mode was set, non-0 means no awb mode was set.
     */
    int getAwbMode(camera_awb_mode_t& awbMode) const;

    /**
     * \brief Set customized cct range.
     *
     * Customized cct range only take effect when awb mode is set to AWB_MODE_MANUAL_CCT_RANGE
     *
     * \param[in] camera_range_t cct range, which specify min and max cct for 3a algorithm to use.
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int setAwbCctRange(camera_range_t cct);

    /**
     * \brief Get customized cct range currently used.
     *
     * \param[out] camera_range_t& cct range
     *
     * \return 0 if cct range was set, non-0 means no cct range was set.
     */
    int getAwbCctRange(camera_range_t& cct) const;

    /**
     * \brief Set customized awb gains.
     *
     * Customized awb gains only take effect when awb mode is set to AWB_MODE_MANUAL_GAIN
     *
     * The range of each gain is (0, 255).
     *
     * \param[in] camera_awb_gains_t awb gains, which specify r,g,b gains for overriding awb result.
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int setAwbGains(camera_awb_gains_t awbGains);

    /**
     * \brief Get customized awb gains currently used.
     *
     * \param[out] camera_awb_gains_t& awb gains
     *
     * \return 0 if awb gain was set, non-0 means no awb gain was set.
     */
    int getAwbGains(camera_awb_gains_t& awbGains) const;

    /**
     * \brief Set awb gain shift.
     *
     * Customized awb gain shift only take effect when awb mode is NOT set to AWB_MODE_MANUAL_GAIN
     *
     * The range of each gain shift is (0, 255).
     *
     * \param[in] camera_awb_gains_t awb gain shift, which specify r,g,b gains for updating awb result.
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int setAwbGainShift(camera_awb_gains_t awbGainShift);

    /**
     * \brief Get customized awb gains shift currently used.
     *
     * \param[out] camera_awb_gains_t& awb gain shift
     *
     * \return 0 if awb gain shift was set, non-0 means no awb gain shift was set.
     */
    int getAwbGainShift(camera_awb_gains_t& awbGainShift) const;

    /**
     * \brief Set manual white point coordinate.
     *
     * Only take effect when awb mode is set to AWB_MODE_MANUAL_WHITE_POINT.
     * The coordinate system is based on frame which is currently displayed.
     *
     * \param[in] camera_coordinate_t white point
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int setAwbWhitePoint(camera_coordinate_t whitePoint);

    /**
     * \brief Get manual white point coordinate.
     *
     * \param[out] camera_coordinate_t& white point
     *
     * \return 0 if white point was set, non-0 means no white point was set.
     */
    int getAwbWhitePoint(camera_coordinate_t& whitePoint) const;

    /**
     * \brief Set customized color transform which is a 3x3 matrix.
     *
     *  Manual color transform only takes effect when awb mode set to AWB_MODE_MANUAL_COLOR_TRANSFORM.
     *
     * \param[in] camera_color_transform_t colorTransform: a 3x3 matrix for color convertion.
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int setColorTransform(camera_color_transform_t colorTransform);

    /**
     * \brief Get color transform matrix currently used.
     *
     * \param[out] camera_color_transform_t& color transform matrix
     *
     * \return 0 if color transform matrix was set, non-0 means no color transform matrix was set.
     */
    int getColorTransform(camera_color_transform_t& colorTransform) const;

    int setAwbRegions(camera_window_list_t awbRegions);
    int getAwbRegions(camera_window_list_t& awbRegions) const;

    int setNrMode(camera_nr_mode_t nrMode);
    int getNrMode(camera_nr_mode_t& nrMode) const;

    int setNrLevel(camera_nr_level_t level);
    int getNrLevel(camera_nr_level_t& level) const;

    int setIrisMode(camera_iris_mode_t irisMode);
    int getIrisMode(camera_iris_mode_t& irisMode);

    int setIrisLevel(int level);
    int getIrisLevel(int& level);

    /**
     * \brief Set WDR mode
     *
     * \param[in] camera_wdr_mode_t wdrMode
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int setWdrMode(camera_wdr_mode_t wdrMode);

    /**
     * \brief Get WDR mode currently used.
     *
     * \param[out] camera_wdr_mode_t& wdrMode
     *
     * \return 0 if awb mode was set, non-0 means no awb mode was set.
     */
    int getWdrMode(camera_wdr_mode_t& wdrMode) const;

    int setWdrLevel(int level);
    int getWdrLevel(int& level) const;

    int setSceneMode(camera_scene_mode_t sceneMode);
    int getSceneMode(camera_scene_mode_t& sceneMode) const;

    int setWeightGridMode(camera_weight_grid_mode_t weightGridMode);
    int getWeightGridMode(camera_weight_grid_mode_t& weightGridMode) const;

    int setBlcAreaMode(camera_blc_area_mode_t blcAreaMode);
    int getBlcAreaMode(camera_blc_area_mode_t& blcAreaMode) const;

    int setFpsRange(camera_range_t fpsRange);
    int getFpsRange(camera_range_t& fpsRange) const;

    /**
     * \brief Set customized effects.
     *
     * One of sharpness, brightness, contrast, hue, saturation could be controlled by this API.
     * Valid range should be [-128, 127]
     *
     * \param[in] camera_image_enhancement_t effects
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int setImageEnhancement(camera_image_enhancement_t effects);

    /**
     * \brief Get customized effects.
     *
     * \param[out] effects
     *
     * \return 0 if effects was set, non-0 return value means no effects was set.
     */
    int getImageEnhancement(camera_image_enhancement_t& effects) const;

    /**
     * \brief Set deinterlace mode
     *
     * \param[in] camera_deinterlace_mode_t deinterlaceMode
     *
     * Setting deinterlace mode only takes effect before camera_device_config_streams called
     * That's it cannot be changed after camera_device_config_streams.
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int setDeinterlaceMode(camera_deinterlace_mode_t deinterlaceMode);

    /**
     * \brief Get deinterlace mode
     *
     * \param[out] camera_deinterlace_mode_t& deinterlaceMode
     *
     * \return 0 if deinterlace mode was set, non-0 means no deinterlace mode was set.
     */
    int getDeinterlaceMode(camera_deinterlace_mode_t &deinterlaceMode) const;

    // Belows are camera capability related parameters operations
    int getSupportedFpsRange(camera_range_array_t& ranges) const;
    int getSupportedStreamConfig(stream_array_t& config) const;

    /**
     * \brief Get supported feature list.
     *
     * Camera application MUST check if the feature is supported before trying to enable it.
     * Otherwise the behavior is undefined currently, HAL may just ignore the request.
     *
     * \param[out] camera_features_list_t& features: All supported feature will be filled in "features"
     *
     * \return: If no feature supported, features will be empty
     */
    int getSupportedFeatures(camera_features_list_t& features) const;

    /**
     * \brief Get ae compensation range supported by camera device
     *
     * \param[out] camera_range_t& evRange
     *
     * \return 0 if ae compensation supported, non-0 or evRange equals [0, 0] means ae compensation not supported.
     */
    int getAeCompensationRange(camera_range_t& evRange) const;

    /**
     * \brief Get ae compensation step supported by camera device
     *
     * Smallest step by which the exposure compensation can be changed.
     * This is the unit for setAeCompensation. For example, if this key has
     * a value of `1/2`, then a setting of `-2` for setAeCompensation means
     * that the target EV offset for the auto-exposure routine is -1 EV.
     *
     * One unit of EV compensation changes the brightness of the captured image by a factor
     * of two. +1 EV doubles the image brightness, while -1 EV halves the image brightness.
     *
     * \param[out] camera_rational_t& evStep
     *
     * \return 0 if ae compensation supported, non-0 means ae compensation not supported.
     */
    int getAeCompensationStep(camera_rational_t& evStep) const;

    /**
     * \brief Get supported manual exposure time range
     *
     * Different sensors or same sensor in different settings may have different supported exposure
     * time range, so camera application needs to use this API to check if the user's settings is
     * in the supported range, if application pass an out of exposure time, HAL will clip it
     * according to this supported range.
     *
     * \param[out] vector<camera_ae_exposure_time_range_t>& etRanges
     *
     * \return 0 if exposure time range is filled by HAL.
     */
    int getSupportedAeExposureTimeRange(std::vector<camera_ae_exposure_time_range_t>& etRanges) const;

    /**
     * \brief Get supported manual sensor gain range
     *
     * Different sensors or same sensor in different settings may have different supported sensor
     * gain range, so camera application needs to use this API to check if the user's settings is
     * in the supported range, if application pass an out of range gain, HAL will clip it according
     * to this supported range.
     *
     * \param[out] vector<camera_ae_gain_range_t>& gainRanges
     *
     * \return 0 if exposure time range is filled by HAL.
     */
    int getSupportedAeGainRange(std::vector<camera_ae_gain_range_t>& gainRanges) const;

    /**
     * \brief Set customized Ae converge speed.
     *
     * \param[in] camera_converge_speed_t speed: the converge speed to be set.
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int setAeConvergeSpeed(camera_converge_speed_t speed);

    /**
     * \brief Get customized Ae converge speed.
     *
     * \param[out] camera_converge_speed_t& speed: the converge speed been set.
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int getAeConvergeSpeed(camera_converge_speed_t& speed) const;

    /**
     * \brief Set customized Awb converge speed.
     *
     * \param[in] camera_converge_speed_t speed: the converge speed to be set.
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int setAwbConvergeSpeed(camera_converge_speed_t speed);

    /**
     * \brief Get customized Awb converge speed.
     *
     * \param[out] camera_converge_speed_t& speed: the converge speed been set.
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int getAwbConvergeSpeed(camera_converge_speed_t& speed) const;

    /**
     * \brief Set customized Ae converge speed mode.
     *
     * \param[in] camera_converge_speed_mode_t mode: the converge speed mode to be set.
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int setAeConvergeSpeedMode(camera_converge_speed_mode_t mode);

    /**
     * \brief Get customized Ae converge speed mode.
     *
     * \param[out] camera_converge_speed_mode_t mode: the converge speed mode to be set.
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int getAeConvergeSpeedMode(camera_converge_speed_mode_t& mode) const;

    /**
     * \brief Set customized Awb converge speed mode.
     *
     * \param[in] camera_converge_speed_mode_t mode: the converge speed mode to be set.
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int setAwbConvergeSpeedMode(camera_converge_speed_mode_t mode);

    /**
     * \brief Get customized Awb converge speed mode.
     *
     * \param[out] camera_converge_speed_mode_t mode: the converge speed mode to be set.
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int getAwbConvergeSpeedMode(camera_converge_speed_mode_t& mode) const;

    /**
     * \brief Set Custom Aic Param
     *
     * \param[in] const void* data: the pointer of data.
     * \param[in] int length: the length of the data.
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int setCustomAicParam(const void* data, unsigned int length);

    /**
     * \brief Get Custom Aic Param
     *
     * \param[out] void* data: the pointer of destination buffer.
     * \param[in/out] in: the buffer size; out: the buffer used size.
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int getCustomAicParam(void* data, unsigned int* length) const;

    /**
     * \brief Set AE distribution priority.
     *
     * \param[in] camera_ae_distribution_priority_t priority: the AE distribution priority to be set.
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int setAeDistributionPriority(camera_ae_distribution_priority_t priority);

    /**
     * \brief Set YUV color range mode
     *
     * \param[in] camera_yuv_color_range_mode_t colorRange: the YUV color range mode to be set.
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int setYuvColorRangeMode(camera_yuv_color_range_mode_t colorRange);

    /**
     * \brief Get YUV color range mode
     *
     * \param[out] camera_yuv_color_range_mode_t colorRange: the YUV color range mode.
     *
     * \return 0 if get successfully, otherwise non-0 value is returned.
     */
    int getYuvColorRangeMode(camera_yuv_color_range_mode_t & colorRange) const;

    /**
     * \brief Get AE distribution priority.
     *
     * \param[out] camera_ae_distribution_priority_t priority: the AE distribution priority.
     *
     * \return 0 if set successfully, otherwise non-0 value is returned.
     */
    int getAeDistributionPriority(camera_ae_distribution_priority_t& priority) const;

    int getJpegQuality(int &quality) const;
    int setJpegQuality(int quality);

    int getJpegThumbnailQuality(int &quality) const;
    int setJpegThumbnailQuality(int quality);

    int getJpegRotation(int &rotation) const;
    int setJpegRotation(int  rotation);

    int setJpegGpsCoordinates(double *coordinates);
    int getJpegGpsLatitude(double &latitude) const;
    int getJpegGpsLongitude(double &longitude) const;
    int getJpegGpsAltitude(double &altiude) const;

    int getJpegGpsTimeStamp(int64_t &timestamp) const;
    int setJpegGpsTimeStamp(int64_t  timestamp);

    int getJpegGpsProcessingMethod(int &processMethod) const;
    int setJpegGpsProcessingMethod(int  processMethod);

    int getImageEffect(camera_effect_mode_t &effect) const;
    int setImageEffect(camera_effect_mode_t  effect);

    int getVideoStabilizationMode(camera_video_stabilization_mode_t &mode) const;
    int setVideoStabilizationMode(camera_video_stabilization_mode_t mode);

    int updateDebugLevel();
private:
    const CameraMetadata* getConstPtr() const
            { return const_cast<const CameraMetadata*>(mMetadata); }

    int setRegions(camera_window_list_t regions, int tag);
    int getRegions(camera_window_list_t& regions, int tag) const;

private:
    CameraMetadata* mMetadata; // Internal data structure
    RWLock* mRwLock;           // Read-write lock to make this class thread-safe
    // parameters for which conversion is supported are stored here
    int mEv;
    int mFps;
    camera_video_stabilization_mode_t mDvsMode;
    camera_ae_mode_t mAeMode;
    int64_t mExposureTime;
    camera_antibanding_mode_t mBandingMode;

}; // class Parameters
/*******************End of Camera Parameters Definition**********************/

}  // namespace icamera

#endif // _CAMERA_PARAMETERS_H_
