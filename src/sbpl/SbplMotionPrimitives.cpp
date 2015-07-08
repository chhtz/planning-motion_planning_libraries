#include "SbplMotionPrimitives.hpp"

#include <set>

namespace motion_planning_libraries {

SbplMotionPrimitives::SbplMotionPrimitives() : mConfig(), mListPrimitivesAngle0(),
        mListPrimitives(), mRadPerDiscreteAngle(0), mPrim_id2Speed()
{
}
    
SbplMotionPrimitives::SbplMotionPrimitives(struct MotionPrimitivesConfig config) : mConfig(config),
        mListPrimitivesAngle0(), mListPrimitives(), mRadPerDiscreteAngle(0), mPrim_id2Speed()
{
    mRadPerDiscreteAngle = (M_PI*2.0) / (double)mConfig.mNumAngles;
}

SbplMotionPrimitives::~SbplMotionPrimitives() {
}

/**
    * Fills mListPrimitives.
    */
void SbplMotionPrimitives::createPrimitives() {  
    // TODO: Fix/check this!
    if(mConfig.mNumPrimPartition != 1 &&
        mConfig.mNumPrimPartition != 2 &&
        mConfig.mNumPrimPartition != 4 &&
        mConfig.mNumPrimPartition != 8) {
        LOG_WARN("Currently only 1, 2, 4 or 8 are valid for mNumPrimPartition!");
    }
    
    if(!mConfig.mMobility.isSet()) {
        LOG_WARN("No primitives will be created, all multipliers within the mobility struct are 0");
        return;
    }
    
    std::vector<struct Primitive> prim_angle_0 = createMPrimsForAngle0();
    createMPrims(prim_angle_0); // Stores to global prim list mListPrimitives as well.
    createIntermediatePoses(mListPrimitives); // Adds intermediate poses.
}

/**
 * Creates unit vectors for all movements respectively discrete minimal turning radius for curves. 
 * In the next step these vectors are rotated (discrete angles) and extended until
 * mNumPrimPartition valid prims have been collected. A prim is valid if it
 * is close enough to a discrete state and if this discrete state is not already reached
 * by another prim.
 */
std::vector<struct Primitive> SbplMotionPrimitives::createMPrimsForAngle0() { 
    
    mListPrimitivesAngle0.clear();
    mPrim_id2Speed.clear();
        
    int primId = 0;
    Primitive prim;
    
    assert (mConfig.mNumPrimPartition >= 1);
    
    // Forward
    if(mConfig.mMobility.mMultiplierForward > 0) {
        prim = Primitive(primId, 
                0, 
                base::Vector3d(1.0, 0.0, 0.0),
                mConfig.mMobility.mMultiplierForward, 
                MOV_FORWARD);
        mListPrimitivesAngle0.push_back(prim);
        mPrim_id2Speed.push_back(mConfig.mMobility.mSpeed);
        primId++;
    }
    
    // Backward
    if(mConfig.mMobility.mMultiplierBackward > 0) {
        prim = Primitive(primId,
                0,
                base::Vector3d(-1.0, 0.0, 0.0),
                mConfig.mMobility.mMultiplierBackward, 
                MOV_BACKWARD);
        mListPrimitivesAngle0.push_back(prim);
        mPrim_id2Speed.push_back(-mConfig.mMobility.mSpeed);
        primId++;         
    }
    
    // Lateral
    if(mConfig.mMobility.mMultiplierLateral > 0) {
        // Lateral Left
        prim = Primitive(primId, 
                0,
                base::Vector3d(0.0, 1.0, 0.0),
                mConfig.mMobility.mMultiplierLateral,
                MOV_LATERAL);
        mListPrimitivesAngle0.push_back(prim);
        mPrim_id2Speed.push_back(mConfig.mMobility.mSpeed);
        primId++;
        
        // Lateral Right
        prim = Primitive(primId, 
                0,
                base::Vector3d(0.0, -1.0, 0.0),
                mConfig.mMobility.mMultiplierLateral, 
                MOV_LATERAL);
        mListPrimitivesAngle0.push_back(prim);
        mPrim_id2Speed.push_back(mConfig.mMobility.mSpeed);
        primId++;
    }
    
    if(mConfig.mMobility.mMultiplierPointTurn > 0) {
        // Pointturn
        prim = Primitive(primId,
                0,
                base::Vector3d(0.0, 0.0, 1.0),
                mConfig.mMobility.mMultiplierPointTurn,
                MOV_POINTTURN);
        mListPrimitivesAngle0.push_back(prim);
        mPrim_id2Speed.push_back(mConfig.mMobility.mSpeed);
        primId++;
        
        prim = Primitive(primId,
                0,
                base::Vector3d(0.0, 0.0, -1.0),
                mConfig.mMobility.mMultiplierPointTurn,
                MOV_POINTTURN);
        mListPrimitivesAngle0.push_back(prim);
        mPrim_id2Speed.push_back(mConfig.mMobility.mSpeed);
        primId++;
    }
          
    // Forward and Backward Curves.
    // Calculates the minimal turning radius in grids.
    double start_turning_radius = std::max(1.0, mConfig.mMobility.mMinTurningRadius / mConfig.mGridSize);
    
    if(mConfig.mMobility.mMultiplierForwardTurn) {
        // Forward left hand bend
        prim = Primitive(primId, 
            0,
            base::Vector3d(0.0, 0.0, 1.0),
            mConfig.mMobility.mMultiplierForwardTurn,
            MOV_FORWARD_TURN);
        prim.mCenterOfRotation = base::Vector3d(0.0, start_turning_radius, 0.0);
        mListPrimitivesAngle0.push_back(prim);
        mPrim_id2Speed.push_back(mConfig.mMobility.mSpeed);
        primId++;
        
        // Forward right hand bend
        prim = Primitive(primId, 
            0,
            base::Vector3d(0.0, 0.0, -1.0),
            mConfig.mMobility.mMultiplierForwardTurn,
            MOV_FORWARD_TURN);
        prim.mCenterOfRotation = base::Vector3d(0.0, -start_turning_radius, 0.0);
        mListPrimitivesAngle0.push_back(prim);
        mPrim_id2Speed.push_back(mConfig.mMobility.mSpeed);
        primId++;
    }
    
    if(mConfig.mMobility.mMultiplierBackwardTurn > 0) {
        // Backward left hand bend
        prim = Primitive(primId, 
            0,
            base::Vector3d(0.0, 0.0, 1.0),
            mConfig.mMobility.mMultiplierBackwardTurn,
            MOV_BACKWARD_TURN);
        prim.mCenterOfRotation = base::Vector3d(0.0, -start_turning_radius, 0.0);
        mListPrimitivesAngle0.push_back(prim);
        mPrim_id2Speed.push_back(-mConfig.mMobility.mSpeed);
        primId++;
        
        // Backward right hand bend
        prim = Primitive(primId, 
            0,
            base::Vector3d(0.0, 0.0, -1.0),
            mConfig.mMobility.mMultiplierBackwardTurn,
            MOV_BACKWARD_TURN);
        prim.mCenterOfRotation = base::Vector3d(0.0, start_turning_radius, 0.0);
        mListPrimitivesAngle0.push_back(prim);
        mPrim_id2Speed.push_back(-mConfig.mMobility.mSpeed);
        primId++;  
    }
    
    // From each base prim mNumPrimPartition prims will be created.
    // So each speed value will be repeated mNumPrimPartition times.
    std::vector<double> mPrim_id2Speed_tmp;
    for(unsigned int i=0; i<mPrim_id2Speed.size(); i++) {
        for(int j=0; j<mConfig.mNumPrimPartition; j++) {
            mPrim_id2Speed_tmp.push_back(mPrim_id2Speed[i]);
        }
    }
    mPrim_id2Speed = mPrim_id2Speed_tmp;
    
    return mListPrimitivesAngle0;
}

/**
    * Uses the passed list of angle 0 discrete-double motion primitives to
    * calculate all primitives. This is done by rotating the angle 0 prims
    * mNumAngles-1 times to cover the complete 2*M_PI and to find the discrete
    * pose.
    */
std::vector<struct Primitive> SbplMotionPrimitives::createMPrims(std::vector<struct Primitive> prims_angle_0) {

    // Creates discrete end poses for all angles.
    mListPrimitives.clear();
    base::Vector3d vec_tmp;
    base::Vector3d turned_center_of_rotation;
    base::Vector3d turned_end_position;
    double theta_tmp = 0.0;
    double max_dist_to_center_grids = 100;
    
    assert(mConfig.mNumAngles != 0);
    
    std::stringstream ss;
    
    // Runs through all discrete angles (default 16)
    for(unsigned int angle=0; angle < mConfig.mNumAngles; ++angle) {
        std::vector< struct Primitive >::iterator it = prims_angle_0.begin();
        
        // Runs through all endposes in grid-local which have been defined for angle 0.
        for(; it != prims_angle_0.end(); ++it) {
            
            ss << "Use primitives " << it->toString() << " to create the prim for angle " << angle << std::endl;
            
            // Extract x,y,theta from the Vector3d.
            turned_end_position = it->mEndPose;
            theta_tmp = turned_end_position[2];
            turned_end_position[2] = 0;
            turned_end_position = Eigen::AngleAxis<double>(angle * mRadPerDiscreteAngle, 
                    Eigen::Vector3d::UnitZ()) * turned_end_position;
            
            // Turn center of rotation vector as well
            turned_center_of_rotation = Eigen::AngleAxis<double>(angle * mRadPerDiscreteAngle, 
                    Eigen::Vector3d::UnitZ()) * it->mCenterOfRotation;
                    
            base::Vector3d discrete_end_pose;
            base::Vector3d discrete_end_pose_rounded;
            int discrete_angle = 0;
            double d = 0.0;
            int upper_discrete_angle = ceil(mConfig.mNumAngles / 4);
            int current_discrete_angle = 1; //upper_discrete_angle;
            std::set< struct Triple > reached_end_positions;
            int prims_added = 0;
            base::Vector3d scaled_center_of_rotation;
            
            while(prims_added < mConfig.mNumPrimPartition) {
                // For each base prim mNumPrimPartition primitives should be created.
                // Even if for some base prims not all sub-prims could be created we
                // have to take care that correct ids are assigned.
                int id = it->mId * mConfig.mNumPrimPartition + prims_added;
                
                discrete_end_pose.setZero();
                discrete_end_pose_rounded.setZero();
                
                switch(it->mMovType) {
                    // Scales the received vector by 1.0, 1.1, ...
                    case MOV_FORWARD:
                    case MOV_BACKWARD:
                    case MOV_LATERAL: {
                        discrete_end_pose = turned_end_position * (1.0 + d);
                        discrete_angle = angle;
                        d += 0.1;
                        break;
                    }
                    // Increaes the angle by one discrete step in one direction.
                    case MOV_POINTTURN: {
                        discrete_end_pose[0] = 0.0;
                        discrete_end_pose[1] = 0.0;
                        // theta_tmp defines the turning direction.
                        discrete_angle = (d+1) * theta_tmp + angle;
                        d++;
                        break;
                    }
                    // First rotates the endpose from ceil(mConfig.mNumAngles / 4) to 1
                    // and after that the vector is scaled.
                    case MOV_FORWARD_TURN:
                    case MOV_BACKWARD_TURN: {
                        // theta_tmp defines the turning direction.
                        double angle_rad = current_discrete_angle * theta_tmp * mRadPerDiscreteAngle;
                        ss << "Turning angle in rad " << angle_rad << std::endl;
                        // TODO Scaling depends of the initial turning radius length, always small steps should be used
                        scaled_center_of_rotation = turned_center_of_rotation  * (1.0 + d);
                        ss << "Scaled center of rotation " << scaled_center_of_rotation.transpose() << std::endl;
                        discrete_end_pose -= scaled_center_of_rotation;
                        discrete_end_pose = Eigen::AngleAxis<double>(angle_rad, Eigen::Vector3d::UnitZ()) * discrete_end_pose;
                        discrete_end_pose += scaled_center_of_rotation;
                        ss << "Discrete end pose " << discrete_end_pose.transpose() << std::endl;
                         // Discrete orientation can be < 0 and > mNumAngles. Will be stored for intermediate point calculation.
                        discrete_angle = current_discrete_angle * theta_tmp + angle;
                        ss << "Discrete angle " << discrete_angle << std::endl;
                        current_discrete_angle++;
                        // Test from small to large angles and increases the vector length afterwards.
                        if(current_discrete_angle > upper_discrete_angle) {
                            current_discrete_angle = 1;//upper_discrete_angle;
                            d += 0.1;
                        }
                        break;
                    }
                    default: {
                    
                        break;
                    }
                }
                
                discrete_end_pose_rounded[0] = round(discrete_end_pose[0]);
                discrete_end_pose_rounded[1] = round(discrete_end_pose[1]);
                // Stores diff between rounded and not rounded end pose and calculates
                // the value of the position (distance to the next discrete grid cell).
                base::Vector3d diff_end_to_rounded = discrete_end_pose_rounded - discrete_end_pose;
                double value_end_position = diff_end_to_rounded.norm();
                
                ss << "New discrete end pose " << discrete_end_pose_rounded.transpose() << 
                        ", discrete_angle " << discrete_angle << ", value " << value_end_position << std::endl; 
                        
                // Due to discretization we have to adapt the the center of rotation.
                // Otherwise the orientation in the end pose may not match a discrete one anymore.
                // For that we have to find the intersection of the orthogonal lines of start and end pose.
                // First Transform end to start.
                        
                        
                // Used to check the curves: Turned back discrete end position have 
                // to be on the same side like the center of rotation and greater 0.
                base::Vector3d discrete_pose_rotate_back;
                discrete_pose_rotate_back.setZero();
                discrete_pose_rotate_back = Eigen::AngleAxis<double>(-angle * mRadPerDiscreteAngle, 
                        Eigen::Vector3d::UnitZ()) * discrete_end_pose_rounded;
                        
                // Checks.
                // Close enugh to a discrete position?
                if(value_end_position > mConfig.mPrimAccuracy) {
                    ss << "Primitive not close enough to a discrete position" << std::endl;
                    continue;
                }
                
                // If it is a curve its discretized end position must not be 0 and 
                // it has to lay on the same side of the x-axis as the center of rotation.
                bool curve_valid = (it->mCenterOfRotation[1] > 0 && discrete_pose_rotate_back[1] > 0) ||
                        (it->mCenterOfRotation[1] < 0 && discrete_pose_rotate_back[1] < 0);
                if((it->mMovType == MOV_FORWARD_TURN || it->mMovType == MOV_BACKWARD_TURN) && !curve_valid) {
                    ss << "Curve is not valid, y of the turned back curve: " << discrete_pose_rotate_back[1] << std::endl;
                    continue;
                }
                
                // Pointturns should cover 90 degree but not much more.
                if(it->mMovType == MOV_POINTTURN && (d+1) > upper_discrete_angle) {
                    ss << "Pointturn primitives should not exceed mConfig.mNumAngles / 4 (rounded up)" << std::endl;
                    break;
                }
                
                // If dist to center exceeds a certain value we have to skip.
                if(discrete_end_pose.norm() > max_dist_to_center_grids) {
                    ss << "Primitive becomes too long, only " << prims_added << 
                            " prims have been found for angle " << angle << " / prim id " << id << std::endl;
                    break;
                }
                
                // Prim not already added?   
                std::pair<std::set< struct Triple >::iterator,bool> set_ret;
                set_ret = reached_end_positions.insert(Triple((int)discrete_end_pose_rounded[0], 
                        (int)discrete_end_pose_rounded[1], discrete_angle));
                if(set_ret.second) { // New element inserted.
                    ss << "New primitive added" << std::endl;
                    Primitive prim_discrete(id, angle, discrete_end_pose_rounded, it->mCostMultiplier, it->mMovType);
                    // The orientation of the discrete endpose still can exceed the borders 0 to mNumAngles.
                    // We will store this for the intermediate point calculation, but the orientation
                    // of the discrete end pose will be truncated to [0,mNumAngles).
                    prim_discrete.setDiscreteEndOrientation(discrete_angle, mConfig.mNumAngles);
                    // Applies the discretization difference to the center of rotation.
                    // TODO check
                    prim_discrete.mCenterOfRotation = scaled_center_of_rotation;// + diff_end_to_rounded;
                    mListPrimitives.push_back(prim_discrete);
                    prims_added++;
                } else {
                    ss << "Primitive with this discrete end positionis already available" << std::endl;
                }
            } 
        }
    }
    LOG_DEBUG("%s\n", ss.str().c_str());
    return mListPrimitives;
}

/**
    * Runs through all the discrete motion primitives and adds the
    * non discrete intermediate poses. This is done with the non truncated
    * end orientation stored within the primitive structure.
    */
void SbplMotionPrimitives::createIntermediatePoses(std::vector<struct Primitive>& discrete_mprims) {

    std::vector<struct Primitive>::iterator it = discrete_mprims.begin();
    base::Vector3d end_pose_local;
    end_pose_local.setZero();
    base::Vector3d center_of_rotation_local;
    center_of_rotation_local.setZero();
    base::Vector3d intermediate_pose;
    double x_step=0.0, y_step=0.0, theta_step=0.0;
    int discrete_rot_diff = 0;
    double start_orientation_local = 0.0;
    
    // Turn variables.
    base::samples::RigidBodyState rbs_cor;
    base::Vector3d base_local;
    base::Affine3d cor2base;
    base::Affine3d base2cor;
    double len_diff_delta = 0.0;
    double len_scale_factor = 0.0;
    double angle_delta = 0.0;
    double len_base_local = 0.0;
    
    std::stringstream ss;
    
    for(;it != discrete_mprims.end(); it++) {
        ss << std::endl << "Create intermediate poses for prim " << it->toString() << std::endl;
     
        start_orientation_local = it->mStartAngle * mRadPerDiscreteAngle;
    
        // Theta range is 0 to 15, have to be sure to use the shortest rotation.
        // And of course the starting orientation has to be regarded!
        discrete_rot_diff = it->getDiscreteEndOrientationNotTruncated() - it->mStartAngle;
        
        end_pose_local[0] = it->mEndPose[0] * mConfig.mGridSize;
        end_pose_local[1] = it->mEndPose[1] * mConfig.mGridSize;
        end_pose_local[2] = 0;
        
        x_step = end_pose_local[0] / ((double)mConfig.mNumPosesPerPrim-1);
        y_step = end_pose_local[1] / ((double)mConfig.mNumPosesPerPrim-1);
        theta_step = (discrete_rot_diff * mRadPerDiscreteAngle) / ((double)mConfig.mNumPosesPerPrim-1);
        
        if(it->mMovType == MOV_FORWARD_TURN || it->mMovType == MOV_BACKWARD_TURN) {
            // Transform center of rotatin to grid local.
            center_of_rotation_local[0] = it->mCenterOfRotation[0] * mConfig.mGridSize;
            center_of_rotation_local[1] = it->mCenterOfRotation[1] * mConfig.mGridSize;
            center_of_rotation_local[2] = 0;
            
            ss << "center of rotation: " << center_of_rotation_local.transpose() << std::endl;
            
            // Create transformation center of rotation to base
            rbs_cor.position = center_of_rotation_local;
            rbs_cor.orientation = Eigen::AngleAxis<double>(start_orientation_local, Eigen::Vector3d::UnitZ());
            cor2base = rbs_cor.getTransform();
            base2cor = cor2base.inverse();
            
            // Transform base (0,0) and endpose_local into the center of rotation frame.
            base_local.setZero();
            base_local = base2cor * base_local;
            end_pose_local = base2cor * end_pose_local;
            
            ss << "base vector: " << base_local.transpose() << ", end vector: " << end_pose_local.transpose() << std::endl;  
            
            len_base_local = base_local.norm();
            double len_end_pose_local = end_pose_local.norm();
            double len_diff =  len_end_pose_local - len_base_local;
            len_diff_delta = len_diff / ((double)mConfig.mNumPosesPerPrim-1);
            len_scale_factor = (len_end_pose_local / len_base_local - 1) / ((double)mConfig.mNumPosesPerPrim-1);
            
            ss << "len base: " << len_base_local << ", len end pose local: " << 
                    len_end_pose_local << ", len_delta: " << len_diff_delta << 
                    ", len_scale_factor " << len_scale_factor << std::endl;
            
            // Calculate real (may changed because of discretization) angle between both vectors.
            double angle = acos(base_local.dot(end_pose_local) / (len_base_local * len_end_pose_local));
            // Add the direction of rotation.
            angle = discrete_rot_diff < 0 ? -angle : angle; 
            angle_delta = angle / ((double)mConfig.mNumPosesPerPrim-1);
            
            // TODO Use the old discrete angles for the intermediate orientation.
            // E.g even if we get 92° due to the discretization we turn from 0 to 90 degree.
            // So we will get along the curve small orientation errors but at start and end the correct orientation.
            
            ss << "Angle between vectors: " << angle << ", angle delta " << angle_delta << std::endl;
        }
        
        base::Vector3d intermediate_pose_cof_tmp;
        for(unsigned int i=0; i<mConfig.mNumPosesPerPrim; i++) { 
            switch(it->mMovType) {
                // Forward, backward or lateral movement, orientation does not change.
                case MOV_FORWARD:
                case MOV_BACKWARD:
                case MOV_LATERAL: {
                    intermediate_pose[0] = i * x_step;
                    intermediate_pose[1] = i * y_step;
                    intermediate_pose[2] = start_orientation_local;
                    break;
                }
                case MOV_POINTTURN: {
                    intermediate_pose[0] = end_pose_local[0];
                    intermediate_pose[1] = end_pose_local[1];
                    intermediate_pose[2] = start_orientation_local + i * theta_step;
                    break;
                }
                case MOV_FORWARD_TURN:
                case MOV_BACKWARD_TURN: {
                    // Calculate each intermediate pose within the center of rotation frame.
                    base::samples::RigidBodyState rbs_intermediate;
                    base::Quaterniond cur_rot;
                    cur_rot = Eigen::AngleAxis<double>(i * angle_delta, Eigen::Vector3d::UnitZ());
                    //rbs_intermediate.position = cur_rot * base::Vector3d(0,-(len_base_local + i * len_diff_delta), 0);
                    rbs_intermediate.position = cur_rot *  (base_local * (1.0 + len_scale_factor * i));
                    rbs_intermediate.orientation = cur_rot;
                    intermediate_pose_cof_tmp = rbs_intermediate.position;
                    intermediate_pose_cof_tmp[2] = rbs_intermediate.getYaw();
                    
                    // Transform back into the base frame.
                    rbs_intermediate.setTransform(cor2base * rbs_intermediate.getTransform() );
                    intermediate_pose[0] = rbs_intermediate.position[0];
                    intermediate_pose[1] = rbs_intermediate.position[1];
                    
                    // TODO Hack, just to check the end orientation problem due to discretization.
                    // Problem: There may be no circle between the start and the discretized end pose.
                    // Actually we have to use a straight line and a circle.
                    if(i == mConfig.mNumPosesPerPrim -1 ) {
                        intermediate_pose[2] = it->mEndPose[2] * mRadPerDiscreteAngle;
                    } else {
                        intermediate_pose[2] = rbs_intermediate.getYaw();
                    }
                    break;
                }
                default: {
                    LOG_WARN("Unknown movement type %d during intermediate pose calculation", (int)it->mMovType);
                    break;
                }
            }
            
            // Truncate orientation of intermediate poses to (-PI,PI] (according to OMPL).
            while(intermediate_pose[2] <= -M_PI)
                intermediate_pose[2] += 2*M_PI;
            while(intermediate_pose[2] > M_PI)
                intermediate_pose[2] -= 2*M_PI;
                         
            ss << "Intermediate pose (x,y,theta) has been added: " << 
                    intermediate_pose[0] << ", " << 
                    intermediate_pose[1] << ", " << 
                    intermediate_pose[2] << std::endl << 
                    "(Local: " << 
                    intermediate_pose_cof_tmp[0] << ", " << 
                    intermediate_pose_cof_tmp[1] << ", " << 
                    intermediate_pose_cof_tmp[2] << ")" << 
                    std::endl;
            
            it->mIntermediatePoses.push_back(intermediate_pose);
        }
    }
    LOG_DEBUG("%s", ss.str().c_str());
}

void SbplMotionPrimitives::storeToFile(std::string path) {
    std::ofstream mprim_file;
    mprim_file.open(path.c_str());
    
    mprim_file << "resolution_m: " <<  std::fixed << std::setprecision(6) << mConfig.mGridSize << std::endl;
    mprim_file << "numberofangles: " << mConfig.mNumAngles << std::endl;
    mprim_file << "totalnumberofprimitives: " << mListPrimitives.size() << std::endl;
    
    
    struct Primitive mprim;
    for(unsigned int i=0; i < mListPrimitives.size(); ++i) {
        mprim =  mListPrimitives[i];
        mprim_file << "primID: " << mprim.mId << std::endl;
        mprim_file << "startangle_c: " << mprim.mStartAngle << std::endl;
        mprim_file << "endpose_c: " << (int)mprim.mEndPose[0] << " " << 
                (int)mprim.mEndPose[1] << " " << (int)mprim.mEndPose[2] << std::endl;
        mprim_file << "additionalactioncostmult: " << mprim.mCostMultiplier << std::endl;
        mprim_file << "intermediateposes: " << mprim.mIntermediatePoses.size() << std::endl;
        base::Vector3d v3d;
        for(unsigned int i=0; i < mprim.mIntermediatePoses.size(); ++i) {
            v3d = mprim.mIntermediatePoses[i];
            mprim_file << std::fixed << std::setprecision(4) << 
                    v3d[0] << " " << v3d[1] << " " << v3d[2] << std::endl;
        }
    }
    
    mprim_file.close();
}

/**
 * Creates a curve within the grid space.
 */
bool SbplMotionPrimitives::createCurvePrimForAngle0(double const turning_radius_discrete, 
        double const angle_rad_discrete, 
        int const prim_id, 
        int const multiplier, 
        Primitive& primitive) {
    
    base::Vector3d center_of_rotation(0.0, turning_radius_discrete, 0.0);
    base::Vector3d vec_endpos(0.0, 0.0, 0.0);
    double angle_rad = angle_rad_discrete * (2*M_PI / (double)mConfig.mNumAngles);
    vec_endpos -= center_of_rotation;
    vec_endpos = Eigen::AngleAxis<double>(angle_rad, Eigen::Vector3d::UnitZ()) * vec_endpos;
    vec_endpos += center_of_rotation;
    // Adds the discrete end orientation.
    vec_endpos[2] = angle_rad_discrete;
    
    enum MovementType mov_type = MOV_BACKWARD_TURN;
    if((turning_radius_discrete > 0 && angle_rad > 0) || (turning_radius_discrete < 0 && angle_rad < 0)) {
        mov_type = MOV_FORWARD_TURN;
    }
    
    primitive = Primitive(prim_id, 
        0,
        vec_endpos,  
        multiplier, 
        mov_type);
    // Store center of rotation.
    primitive.mCenterOfRotation = center_of_rotation;
    
    return true;
}

int SbplMotionPrimitives::calcDiscreteEndOrientation(double yaw_rad) {
    int discrete_theta = round(yaw_rad / (double)mConfig.mNumAngles);
    while (discrete_theta >= (int)mConfig.mNumAngles)
        discrete_theta -= mConfig.mNumAngles;
    while (discrete_theta < 0)
        discrete_theta += mConfig.mNumAngles;
    return discrete_theta;
}

bool SbplMotionPrimitives::getSpeed(unsigned int const prim_id, double& speed) {
    if(prim_id >= mPrim_id2Speed.size()) {
        LOG_WARN("Speed for primitive id %d is not available\n", prim_id);
        return false;
    }

    speed = mPrim_id2Speed.at(prim_id);
    return true;
}

// TODO TEST!
bool SbplMotionPrimitives::calculateOrthogonalIntersection(
        base::samples::RigidBodyState start_pose, 
        base::samples::RigidBodyState end_pose,
        base::Vector3d& cof) {
    
    base::Affine3d start2end, end2start;
    
    start2end = start_pose.getTransform();
    end2start = start2end.inverse();
    
    // Calculates end pose within the start frame.
    end_pose.setTransform(end2start * end_pose.getTransform());
    
    double theta_end = end_pose.getYaw();
    if(theta_end == 0) {
        LOG_WARN("Intersection not possible\n");
        return false;
    }
    
    double rot = theta_end + (theta_end > 0 ? M_PI/2.0 : -M_PI/2.0); 
    // Create orthogonal line.
    base::Vector3d ortho_vec(1,0,0);
    ortho_vec = Eigen::AngleAxis<double>(rot, Eigen::Vector3d::UnitZ()) * ortho_vec;
    ortho_vec += end_pose.position;
    
    double t = -end_pose.position[0] / ortho_vec[0];
    
    base::Vector3d cof_tmp = end_pose.position + t * ortho_vec;
    
    // Transform back to base.
    cof = start2end * cof_tmp;
    
    return true;
}
                                                                    
} // end namespace motion_planning_libraries
