/**
* This file is part of DSO.
*
* Copyright 2016 Technical University of Munich and Intel.
* Developed by Jakob Engel <engelj at in dot tum dot de>,
* for more information see <http://vision.in.tum.de/dso>.
* If you use this code, please cite the respective publications as
* listed on the above website.
*
* DSO is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* DSO is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with DSO. If not, see <http://www.gnu.org/licenses/>.
*/


#pragma once
#define MAX_ACTIVE_FRAMES 100


#include "util/globalCalib.h"
#include "vector"

#include <iostream>
#include <fstream>
#include "util/NumType.h"
#include "FullSystem/Residuals.h"
#include "util/ImageAndExposure.h"


namespace fdso
{


inline Vec2 affFromTo(Vec2 from, Vec2 to)	// contains affine parameters as XtoWorld.
{
	return Vec2(from[0] / to[0], (from[1] - to[1]) / to[0]);
}


struct FrameHessian;
struct PointHessian;

class ImmaturePoint;
class FrameShell;

class EFFrame;
class EFPoint;

#define SCALE_IDEPTH 1.0f		// scales internal value to idepth.
#define SCALE_XI_ROT 1.0f
#define SCALE_XI_TRANS 0.5f
#define SCALE_F 50.0f
#define SCALE_C 50.0f
#define SCALE_W 1.0f
#define SCALE_A 10.0f
#define SCALE_B 1000.0f

#define SCALE_IDEPTH_INVERSE (1.0f / SCALE_IDEPTH)
#define SCALE_XI_ROT_INVERSE (1.0f / SCALE_XI_ROT)
#define SCALE_XI_TRANS_INVERSE (1.0f / SCALE_XI_TRANS)
#define SCALE_F_INVERSE (1.0f / SCALE_F)
#define SCALE_C_INVERSE (1.0f / SCALE_C)
#define SCALE_W_INVERSE (1.0f / SCALE_W)
#define SCALE_A_INVERSE (1.0f / SCALE_A)
#define SCALE_B_INVERSE (1.0f / SCALE_B)

/**
 * 每一帧预计算的残差
 */
struct FrameFramePrecalc
{
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
	// static values
	// 状态量
	static int instanceCounter;
	//主导帧
	FrameHessian* host;	// defines row
	//目标帧
	FrameHessian* target;	// defines column

	// precalc values
	// 预计算的值
	Mat33f PRE_RTll;	//R
	Mat33f PRE_KRKiTll;	//K*R*K'
	Mat33f PRE_RKiTll;	//R*K'
	Mat33f PRE_RTll_0;	//R evalPT

	Vec2f PRE_aff_mode;	//主导帧和目标帧之间的光度ａ和ｂ变化
	float PRE_b0_mode;	//主导帧的b

	Vec3f PRE_tTll;	//t
	Vec3f PRE_KtTll;	//R*t
	Vec3f PRE_tTll_0;	//t PRE_RTll_0

	float distanceLL;


	inline ~FrameFramePrecalc() {}
	inline FrameFramePrecalc() {host = target = 0;}
	void set(FrameHessian* host, FrameHessian* target, CalibHessian* HCalib);
};

/**
 * 每一帧的信息
 */
struct FrameHessian
{
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
	//帧的能量函数
	EFFrame* efFrame;

	// constant info & pre-calculated values
	//DepthImageWrap* frame;
    	//每一帧的信息
	FrameShell* shell;

    	//梯度
	Eigen::Vector3f* dI;				 // trace, fine tracking. Used for direction select (not for gradient histograms etc.)
    	//用于跟踪，和初始化
	Eigen::Vector3f* dIp[PYR_LEVELS];	 // coarse tracking / coarse initializer. NAN in [0] only.
    	//用于像素的选择，用直方图，金字塔
	float* absSquaredGrad[PYR_LEVELS];  // only used for pixel select (histograms etc.). no NAN.

    	//图像关键帧ID
	int frameID;						// incremental ID for keyframes only!
	static int instanceCounter;
	//连续的图像id
	int idx;

	// Photometric Calibration Stuff
	// 光度矫正，根据跟踪残差动态设置
	float frameEnergyTH;	// set dynamically depending on tracking residual
 	//即为参数t,用来表示曝光时间
	float ab_exposure;

    	//是否边缘化
	bool flaggedForMarginalization;

	//有效点
	std::vector<PointHessian*> pointHessians;				// contains all ACTIVE points.
	//已边缘化的点
	std::vector<PointHessian*> pointHessiansMarginalized;	// contains all MARGINALIZED points (= fully marginalized, usually because point went OOB.)
	//出界点/外点
	std::vector<PointHessian*> pointHessiansOut;		// contains all OUTLIER points (= discarded.).
	//未成熟的点
	std::vector<PointHessian*> potentialPointHessians;

	//当前帧生成的点
	std::vector<ImmaturePoint*> immaturePoints;		// contains all OUTLIER points (= discarded.).

	//零空间位姿
	Mat66 nullspaces_pose;
	//零空间的光度参数
	Mat42 nullspaces_affine;
	//零空间的尺度
	Vec6 nullspaces_scale;

	// variable info.
	// 相机在世界坐标系的位姿
	SE3 worldToCam_evalPT;
	//零状态
	Vec10 state_zero;
	//尺度状态，实际用的
	Vec10 state_scaled;
	//每次设置的状态，0-5为世界坐标系到相机坐标系的左扰动，6和7为光度参数
	Vec10 state;	// [0-5: worldToCam-leftEps. 6-7: a,b]
	Vec10 step;
	Vec10 step_backup;
	Vec10 state_backup;


	EIGEN_STRONG_INLINE const SE3 &get_worldToCam_evalPT() const {return worldToCam_evalPT;}
	EIGEN_STRONG_INLINE const Vec10 &get_state_zero() const {return state_zero;}
	EIGEN_STRONG_INLINE const Vec10 &get_state() const {return state;}
	EIGEN_STRONG_INLINE const Vec10 &get_state_scaled() const {return state_scaled;}
	EIGEN_STRONG_INLINE const Vec10 get_state_minus_stateZero() const {return get_state() - get_state_zero();}

	// precalc values
	// 预计算的值
	SE3 PRE_worldToCam;
	SE3 PRE_camToWorld;
	//目标帧预计算的值
	std::vector<FrameFramePrecalc, Eigen::aligned_allocator<FrameFramePrecalc>> targetPrecalc;
	//调试图像信息
	MinimalImageB3* debugImage;

	//获取世界坐标系到相机坐标系的左扰动
	inline Vec6 w2c_leftEps() const {return get_state_scaled().head<6>();}
	inline AffLight aff_g2l() const {return AffLight(get_state_scaled()[6], get_state_scaled()[7]);}
	inline AffLight aff_g2l_0() const {return AffLight(get_state_zero()[6] * SCALE_A, get_state_zero()[7] * SCALE_B);}

	/**
	 * [setStateZero description]
	 * @param state_zero [description]
	 * 设置零状态
	 */
	void setStateZero(Vec10 state_zero);
	/**
	 * [setState description]
	 * @param state [description]
	 * 设置状态
	 */
	inline void setState(Vec10 state)
	{
		//设置state
		this->state = state;
		//获取sstate_scaled从第0个开始的3个值
		//设置state_scaled，即设置左扰动w2c_leftEps
		//state与state_scaled差了SCALE
		state_scaled.segment<3>(0) = SCALE_XI_TRANS * state.segment<3>(0);
		state_scaled.segment<3>(3) = SCALE_XI_ROT * state.segment<3>(3);
		state_scaled[6] = SCALE_A * state[6];
		state_scaled[7] = SCALE_B * state[7];
		state_scaled[8] = SCALE_A * state[8];
		state_scaled[9] = SCALE_B * state[9];

		//左扰动的指数映射*worldToCam_evalPT
		PRE_worldToCam = SE3::exp(w2c_leftEps()) * get_worldToCam_evalPT();
		PRE_camToWorld = PRE_worldToCam.inverse();
		//setCurrentNullspace();
	};

	/**
	 * [setStateScaled description]
	 * @param state_scaled [description]
	 * 设置状态的逆
	 */
	inline void setStateScaled(Vec10 state_scaled)
	{
		//设置state_scaled，即设置左扰动w2c_leftEps
		this->state_scaled = state_scaled;

		//设置state
		//state与state_scaled差了SCALE INVERSE
		state.segment<3>(0) = SCALE_XI_TRANS_INVERSE * state_scaled.segment<3>(0);
		state.segment<3>(3) = SCALE_XI_ROT_INVERSE * state_scaled.segment<3>(3);
		state[6] = SCALE_A_INVERSE * state_scaled[6];
		state[7] = SCALE_B_INVERSE * state_scaled[7];
		state[8] = SCALE_A_INVERSE * state_scaled[8];
		state[9] = SCALE_B_INVERSE * state_scaled[9];

		//左扰动的指数映射*worldToCam_evalPT
		PRE_worldToCam = SE3::exp(w2c_leftEps()) * get_worldToCam_evalPT();
		PRE_camToWorld = PRE_worldToCam.inverse();
		//setCurrentNullspace();
	};

	/**
	 * [setEvalPT description]
	 * @param worldToCam_evalPT [description]
	 * @param state             [description]
	 */
	inline void setEvalPT(SE3 worldToCam_evalPT, Vec10 state)
	{
		this->worldToCam_evalPT = worldToCam_evalPT;
		setState(state);
		setStateZero(state);
	};

	/**
	 * [setEvalPT_scaled description]
	 * @param worldToCam_evalPT [description]
	 * @param aff_g2l           [description]
	 */
	inline void setEvalPT_scaled(SE3 worldToCam_evalPT, AffLight aff_g2l)
	{
		Vec10 initial_state = Vec10::Zero();
		initial_state[6] = aff_g2l.a;
		initial_state[7] = aff_g2l.b;
		this->worldToCam_evalPT = worldToCam_evalPT;
		setStateScaled(initial_state);
		setStateZero(this->get_state());
	};

	void release();

	/**
	 * 消除
	 */
	inline ~FrameHessian()
	{
		assert(efFrame == 0);
		release(); instanceCounter--;
		for (int i = 0; i < pyrLevelsUsed; i++)
		{
			delete[] dIp[i];
			delete[]  absSquaredGrad[i];

		}
		if (debugImage != 0) delete debugImage;
	};

	/**
	 * [FrameHessian description]
	 * @return [description]
	 * 初始化
	 */
	inline FrameHessian()
	{
		instanceCounter++;
		flaggedForMarginalization = false;
		frameID = -1;
		efFrame = 0;
		frameEnergyTH = 8 * 8 * patternNum;
		debugImage = 0;
	};

	/**
	 * [makeImages description]
	 * @param color          [description]
	 * @param overexposedMap [description]
	 * @param HCalib         [description]
	 * 构建帧的梯度图，构建帧的雅克比和Hessian矩阵
	 */
	void makeImages(float* color, CalibHessian* HCalib);

	/**
	 * [getPrior description]
	 * @return [description]
	 * 获得先验的
	 */
	inline Vec10 getPrior()
	{
		Vec10 p =  Vec10::Zero();
		if (frameID == 0)
		{
			p.head<3>() = Vec3::Constant(setting_initialTransPrior);
			p.segment<3>(3) = Vec3::Constant(setting_initialRotPrior);
			if (setting_solverMode & SOLVER_REMOVE_POSEPRIOR) p.head<6>().setZero();

			p[6] = setting_initialAffAPrior;
			p[7] = setting_initialAffBPrior;
		}
		else
		{
			if (setting_affineOptModeA < 0)
				p[6] = setting_initialAffAPrior;
			else
				p[6] = setting_affineOptModeA;

			if (setting_affineOptModeB < 0)
				p[7] = setting_initialAffBPrior;
			else
				p[7] = setting_affineOptModeB;
		}
		p[8] = setting_initialAffAPrior;
		p[9] = setting_initialAffBPrior;
		return p;
	}

	/**
	 * [getPriorZero description]
	 * @return [description]
	 * 获得零Hessian
	 */
	inline Vec10 getPriorZero()
	{
		return Vec10::Zero();
	}

};

/**
 * 矫正Hessian矩阵
 */
struct CalibHessian
{
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
	static int instanceCounter;

	VecC value_zero;
	VecC value_scaled;
	VecCf value_scaledf;
	VecCf value_scaledi;
	VecC value;
	VecC step;
	VecC step_backup;
	VecC value_backup;
	VecC value_minus_value_zero;

	inline ~CalibHessian() {instanceCounter--;}
	inline CalibHessian()
	{

		VecC initial_value = VecC::Zero();
		initial_value[0] = fxG[0];
		initial_value[1] = fyG[0];
		initial_value[2] = cxG[0];
		initial_value[3] = cyG[0];

		setValueScaled(initial_value);
		value_zero = value;
		value_minus_value_zero.setZero();

		instanceCounter++;
		for (int i = 0; i < 256; i++)
			Binv[i] = B[i] = i;		// set gamma function to identity
	};


	// normal mode: use the optimized parameters everywhere!
	inline float& fxl() {return value_scaledf[0];}
	inline float& fyl() {return value_scaledf[1];}
	inline float& cxl() {return value_scaledf[2];}
	inline float& cyl() {return value_scaledf[3];}
	inline float& fxli() {return value_scaledi[0];}
	inline float& fyli() {return value_scaledi[1];}
	inline float& cxli() {return value_scaledi[2];}
	inline float& cyli() {return value_scaledi[3];}



	inline void setValue(VecC value)
	{
		// [0-3: Kl, 4-7: Kr, 8-12: l2r]
		this->value = value;
		value_scaled[0] = SCALE_F * value[0];
		value_scaled[1] = SCALE_F * value[1];
		value_scaled[2] = SCALE_C * value[2];
		value_scaled[3] = SCALE_C * value[3];

		this->value_scaledf = this->value_scaled.cast<float>();
		this->value_scaledi[0] = 1.0f / this->value_scaledf[0];
		this->value_scaledi[1] = 1.0f / this->value_scaledf[1];
		this->value_scaledi[2] = - this->value_scaledf[2] / this->value_scaledf[0];
		this->value_scaledi[3] = - this->value_scaledf[3] / this->value_scaledf[1];
		this->value_minus_value_zero = this->value - this->value_zero;
	};

	inline void setValueScaled(VecC value_scaled)
	{
		this->value_scaled = value_scaled;
		this->value_scaledf = this->value_scaled.cast<float>();
		value[0] = SCALE_F_INVERSE * value_scaled[0];
		value[1] = SCALE_F_INVERSE * value_scaled[1];
		value[2] = SCALE_C_INVERSE * value_scaled[2];
		value[3] = SCALE_C_INVERSE * value_scaled[3];

		this->value_minus_value_zero = this->value - this->value_zero;
		this->value_scaledi[0] = 1.0f / this->value_scaledf[0];
		this->value_scaledi[1] = 1.0f / this->value_scaledf[1];
		this->value_scaledi[2] = - this->value_scaledf[2] / this->value_scaledf[0];
		this->value_scaledi[3] = - this->value_scaledf[3] / this->value_scaledf[1];
	};


	float Binv[256];
	float B[256];


	EIGEN_STRONG_INLINE float getBGradOnly(float color)
	{
		int c = color + 0.5f;
		if (c < 5) c = 5;
		if (c > 250) c = 250;
		return B[c + 1] - B[c];
	}

	EIGEN_STRONG_INLINE float getBInvGradOnly(float color)
	{
		int c = color + 0.5f;
		if (c < 5) c = 5;
		if (c > 250) c = 250;
		return Binv[c + 1] - Binv[c];
	}
};


// hessian component associated with one point.
/**
 * 每一个点的Hessian
 */
struct PointHessian
{
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
	static int instanceCounter;

	//点的能量函数
	EFPoint* efPoint;

	// static values
	// 主导帧中这个点的模式灰度值
	float color[MAX_RES_PER_POINT];			// colors in host frame
	//主导帧中这个点的权重
	float weights[MAX_RES_PER_POINT];		// host-weights for respective residuals.

	//点的坐标
	float u, v;
	//点id
	int idx;
	//点的能量阈值
	float energyTH;
	//点对应的主导帧
	FrameHessian* host;
	//是否有先验的深度值
	bool hasDepthPrior;

	//类型
	float my_type;

	//逆深度
	float idepth_scaled;
	float idepth_zero_scaled;
	float idepth_zero;
	float idepth;
	float step;
	float step_backup;
	float idepth_backup;

	float nullspaces_scale;
	float idepth_hessian;
	float maxRelBaseline;
	int numGoodResiduals;

	enum PtStatus {ACTIVE = 0, INACTIVE, OUTLIER, OOB, MARGINALIZED};
	PtStatus status;

	inline void setPointStatus(PtStatus s) {status = s;}


	/**
	 * [setIdepth description]
	 * @param idepth [description]
	 * 设置逆深度
	 */
	inline void setIdepth(float idepth) {
		this->idepth = idepth;
		this->idepth_scaled = SCALE_IDEPTH * idepth;
	}

	/**
	 * [setIdepthScaled description]
	 * @param idepth_scaled [description]
	 * 设置带尺度的逆深度
	 */
	inline void setIdepthScaled(float idepth_scaled) {
		this->idepth = SCALE_IDEPTH_INVERSE * idepth_scaled;
		this->idepth_scaled = idepth_scaled;
	}

	/**
	 * [setIdepthZero description]
	 * @param idepth [description]
	 */
	inline void setIdepthZero(float idepth) {
		idepth_zero = idepth;
		idepth_zero_scaled = SCALE_IDEPTH * idepth;
		nullspaces_scale = -(idepth * 1.001 - idepth / 1.001) * 500;
	}

	//只有好的残差（不是OOB不离群）。任意阶。
	std::vector<PointFrameResidual*> residuals;					// only contains good residuals (not OOB and not OUTLIER). Arbitrary order.
	//包含有关最后两个（的）残差的信息。帧。（[ 0 ] =最新的，[ 1 ] =前一个）。
	std::pair<PointFrameResidual*, ResState> lastResiduals[2]; 	// contains information about residuals to the last two (!) frames. ([0] = latest, [1] = the one before).


	void release();
	PointHessian(const ImmaturePoint* const rawPoint, CalibHessian* Hcalib);
	inline ~PointHessian() {assert(efPoint == 0); release(); instanceCounter--;}


	/**
	 * [isOOB description]
	 * @param  toKeep [description]
	 * @param  toMarg [description]
	 * @return        [description]
	 * 是否是外点
	 */
	inline bool isOOB(const std::vector<FrameHessian*>& toKeep, const std::vector<FrameHessian*>& toMarg) const
	{

		int visInToMarg = 0;
		for (PointFrameResidual* r : residuals)
		{
			if (r->state_state != ResState::IN) continue;
			for (FrameHessian* k : toMarg)
				if (r->target == k) visInToMarg++;
		}
		if ((int)residuals.size() >= setting_minGoodActiveResForMarg &&
		        numGoodResiduals > setting_minGoodResForMarg + 10 &&
		        (int)residuals.size() - visInToMarg < setting_minGoodActiveResForMarg)
			return true;

		if (lastResiduals[0].second == ResState::OOB) return true;
		if (residuals.size() < 2) return false;
		if (lastResiduals[0].second == ResState::OUTLIER && lastResiduals[1].second == ResState::OUTLIER) return true;
		return false;
	}


	/**
	 * [isInlierNew description]
	 * @return [description]
	 * 是否是新的内点
	 */
	inline bool isInlierNew()
	{
		return (int)residuals.size() >= setting_minGoodActiveResForMarg
		       && numGoodResiduals >= setting_minGoodResForMarg;
	}

};

}

