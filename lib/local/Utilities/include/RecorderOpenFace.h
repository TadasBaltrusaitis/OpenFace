///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2017, Tadas Baltrusaitis all rights reserved.
//
// ACADEMIC OR NON-PROFIT ORGANIZATION NONCOMMERCIAL RESEARCH USE ONLY
//
// BY USING OR DOWNLOADING THE SOFTWARE, YOU ARE AGREEING TO THE TERMS OF THIS LICENSE AGREEMENT.  
// IF YOU DO NOT AGREE WITH THESE TERMS, YOU MAY NOT USE OR DOWNLOAD THE SOFTWARE.
//
// License can be found in OpenFace-license.txt
//
//     * Any publications arising from the use of this software, including but
//       not limited to academic journal and conference publications, technical
//       reports and manuals, must cite at least one of the following works:
//
//       OpenFace: an open source facial behavior analysis toolkit
//       Tadas Baltrušaitis, Peter Robinson, and Louis-Philippe Morency
//       in IEEE Winter Conference on Applications of Computer Vision, 2016  
//
//       Rendering of Eyes for Eye-Shape Registration and Gaze Estimation
//       Erroll Wood, Tadas Baltrušaitis, Xucong Zhang, Yusuke Sugano, Peter Robinson, and Andreas Bulling 
//       in IEEE International. Conference on Computer Vision (ICCV),  2015 
//
//       Cross-dataset learning and person-speci?c normalisation for automatic Action Unit detection
//       Tadas Baltrušaitis, Marwa Mahmoud, and Peter Robinson 
//       in Facial Expression Recognition and Analysis Challenge, 
//       IEEE International Conference on Automatic Face and Gesture Recognition, 2015 
//
//       Constrained Local Neural Fields for robust facial landmark detection in the wild.
//       Tadas Baltrušaitis, Peter Robinson, and Louis-Philippe Morency. 
//       in IEEE Int. Conference on Computer Vision Workshops, 300 Faces in-the-Wild Challenge, 2013.    
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __RECORDER_OPENFACE_h_
#define __RECORDER_OPENFACE_h_

#include "RecorderCSV.h"
#include "RecorderHOG.h"
#include "RecorderOpenFaceParameters.h"

// For speeding up writing
#include "tbb/concurrent_queue.h"
#include "tbb/task_group.h"

// System includes
#include <vector>

// OpenCV includes
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

namespace Utilities
{

	//===========================================================================
	/**
	A class for recording data processed by OpenFace (facial landmarks, head pose, facial action units, aligned face, HOG features, and tracked video
	*/
	class RecorderOpenFace {

	public:

		// The constructor for the recorder, need to specify if we are recording a sequence or not, in_filename should be just the name and not contain extensions
		RecorderOpenFace(const std::string in_filename, const RecorderOpenFaceParameters& parameters, std::vector<std::string>& arguments);
		RecorderOpenFace(const std::string in_filename, const RecorderOpenFaceParameters& parameters, std::string output_directory);

		~RecorderOpenFace();

		// Closing and cleaning up the recorder
		void Close();

		// Adding observations to the recorder

		// Required observations for video/image-sequence
		void SetObservationTimestamp(double timestamp);

		// Required observations for video/image-sequence
		void SetObservationFrameNumber(int frame_number);

		// If in multiple face mode, identifying which face was tracked
		void SetObservationFaceID(int face_id);

		// All observations relevant to facial landmarks
		void SetObservationLandmarks(const cv::Mat_<float>& landmarks_2D, const cv::Mat_<float>& landmarks_3D,
			const cv::Vec6f& params_global, const cv::Mat_<float>& params_local, double confidence, bool success);

		// Pose related observations
		void SetObservationPose(const cv::Vec6f& pose);

		// AU related observations
		void SetObservationActionUnits(const std::vector<std::pair<std::string, double> >& au_intensities, 
			const std::vector<std::pair<std::string, double> >& au_occurences);

		// Gaze related observations
		void SetObservationGaze(const cv::Point3f& gazeDirection0, const cv::Point3f& gazeDirection1,
			const cv::Vec2f& gaze_angle, const std::vector<cv::Point2f>& eye_landmarks2D, const std::vector<cv::Point3f>& eye_landmarks3D);

		// Face alignment related observations
		void SetObservationFaceAlign(const cv::Mat& aligned_face);

		// HOG feature related observations
		void SetObservationHOG(bool good_frame, const cv::Mat_<double>& hog_descriptor, int num_cols, int num_rows, int num_channels);

		void SetObservationVisualization(const cv::Mat &vis_track);

		// Write out all observations for current face (except for tracked image/video)
		void WriteObservation();

		// Separate method for writing tracked video observation, this is done because video observation is written once a frame/image, other observations can happen multiple times a frame/image
		void WriteObservationTracked();

		std::string GetCSVFile() { return csv_filename; }

	private:

		// Used to keep track if the recording is still going (for the writing threads)
		bool recording;

		// For keeping track of tasks
		tbb::task_group writing_threads;

		// A thread that will write video output, so that the rest of the application does not block on it
		void VideoWritingTask();
		void AlignedImageWritingTask();

		// Blocking copy, assignment and move operators, as it does not make sense to save to the same location
		RecorderOpenFace & operator= (const RecorderOpenFace& other);
		RecorderOpenFace & operator= (const RecorderOpenFace&& other);
		RecorderOpenFace(const RecorderOpenFace&& other);
		RecorderOpenFace(const RecorderOpenFace& other);

		void PrepareRecording(const std::string& in_filename);

		// Keeping track of what to output and how to output it
		const RecorderOpenFaceParameters params;

		// Keep track of the file and output root location
		std::string record_root;
		std::string default_record_directory = "processed"; // By default we are writing in the processed directory in the working directory, if no output parameters provided
		std::string out_name; // Short name, based on which other names are constructed
		std::string csv_filename;
		std::string aligned_output_directory;
		std::ofstream metadata_file;

		// The actual output file stream that will be written
		RecorderCSV csv_recorder;
		RecorderHOG hog_recorder;

		// The actual temporary storage for the observations
		
		double timestamp;
		int face_id;
		int frame_number;

		// Facial landmark related observations
		cv::Mat_<float> landmarks_2D;
		cv::Mat_<float> landmarks_3D;
		cv::Vec6f pdm_params_global;
		cv::Mat_<float> pdm_params_local;
		double landmark_detection_confidence;
		bool landmark_detection_success;

		// Head pose related observations
		cv::Vec6f head_pose;

		// Action Unit related observations
		std::vector<std::pair<std::string, double> > au_intensities;
		std::vector<std::pair<std::string, double> > au_occurences;

		// Gaze related observations
		cv::Point3f gaze_direction0;
		cv::Point3f gaze_direction1;
		cv::Vec2f gaze_angle;
		std::vector<cv::Point2f> eye_landmarks2D;
		std::vector<cv::Point3f> eye_landmarks3D;

		// For video writing
		cv::VideoWriter video_writer;
		std::string media_filename;
		
		// Do not exceed 100MB in the concurrent queue
		const int TRACKED_QUEUE_CAPACITY = 100;
		cv::Mat vis_to_out;
		tbb::concurrent_bounded_queue<std::pair<std::string, cv::Mat> > vis_to_out_queue;

		// For aligned face writing
		const int ALIGNED_QUEUE_CAPACITY = 100;
		cv::Mat aligned_face;
		tbb::concurrent_bounded_queue<std::pair<std::string, cv::Mat> > aligned_face_queue;

	};
}
#endif