#pragma once
#include <vector>
#include <glm/gtc/matrix_transform.hpp>		//GLM (matrices, vectors)
#define _USE_MATH_DEFINES
#include <math.h>
/**
	Creates a linear test with constant acceleration a0 and initial speed v0. 
	Returns the resulting positions in pointer "resultPositions" (passed as reference)
	Returns the number of position samples in the array. 
*/
size_t createLinearTest(float start[4], float end[4], float v0, float a0, float dt, float ** resultPositions) {
	std::vector<glm::vec3> positions;	
	//0. Get unitary vector in direction of motion
	glm::vec3 A = glm::vec3(start[0], start[1], start[2]), B=glm::vec3(end[0], end[1], end[2]), midPoint = (A + B) / 2.0f;
	glm::vec3 direction = B - A;
	float distance = sqrtf(direction.x*direction.x + direction.y*direction.y + direction.z*direction.z);
	direction /=distance ;
	//1. Declare state: 
	glm::vec3 p_t = A;
	float v_t = v0, a_t=a0;	
	//2. Accelerate part: Accelerate until we reach mid point (dot product becomes negative).
	while (glm::dot(direction,(midPoint - p_t)) > 0) {
		positions.push_back(p_t);
		v_t += a0*dt;
		p_t += v_t*direction*dt;		
	}
	//3. Decelerate part (until we reach B).
	while (glm::dot(direction,(B - p_t)) > 0) {
		positions.push_back(p_t);
		v_t -= a0*dt;
		if (v_t < 0.1f)v_t = 0.1f;//minimum speed 0.1m/s (otherwise, v could become zero, and this loop would never end... this only affects last steps in the test (low speeds)
		p_t += v_t*direction*dt;			
	}
	positions.push_back(B); 
	//4. Create positions array:
	*resultPositions = new float[4 * positions.size()];
	for (size_t i = 0; i < positions.size(); i++){
		(*resultPositions)[4 * i + 0] = positions[i].x;
		(*resultPositions)[4 * i + 1] = positions[i].y;
		(*resultPositions)[4 * i + 2] = positions[i].z;
		(*resultPositions)[4 * i + 3] = 1;
	}
	return positions.size();
}

/**
	Retur closest omega we can simulate
*/
float createCircleTest(float O[3], float radius, float a0, float dt, float** accelPath_data, size_t* sizeAccelPath, float** circlePath_data, size_t* sizeCirclePath, float** decelPath_data, size_t* sizeDecelPath) {
	//0. Compute closest omega that we can actually use, for a cyclic path (avoid discontinuities in last step in path)
	float target_omega = sqrtf(a0 / radius);
	int updatesPerCircle = (int)(2 * M_PI / (target_omega*dt) + 0.5f);
	float closest_omega = (float) (2 * M_PI / (updatesPerCircle*dt));
	float closest_v = closest_omega*radius; 
	//1. Compute circle using closest_omega:
	float startPoint[] = { O[0] + radius, O[1], O[2] };
	std::vector<glm::vec3> circlePositions;	
	for (int s = 0; s < updatesPerCircle; s++) {
		float angle =(float)( (2 * M_PI*s) / updatesPerCircle);
		glm::vec3 curPoint(	   cosf(angle)*startPoint[0]-sinf(angle)*startPoint[1]
							,  sinf(angle)*startPoint[0]+cosf(angle)*startPoint[1]
							,  startPoint[2]);
		circlePositions.push_back(curPoint);
	}
	//2. Compute deceleration path: decelerating circle from startPoint, ending in startPoint
	float cur_omega = closest_omega, angle=0;
	float min_omega = 2 * M_PI;
	float deceleration = 30;//rads/sec^2
	std::vector<glm::vec3> deceleratingPositions, acceleratingPositions;	
	//2.a. Revolve around the circle until angular speed is small enough
	while (cur_omega > min_omega) {
		//Compute curPoint according to angle
		glm::vec3 curPoint(	   cosf(angle)*startPoint[0]-sinf(angle)*startPoint[1]
							,  sinf(angle)*startPoint[0]+cosf(angle)*startPoint[1]
							,  startPoint[2]);
		//Update cur_omega and angle
		cur_omega -= deceleration*dt;
		angle += cur_omega*dt; 
		angle = fmodf(angle, 2 * M_PI);
		//Add position
		deceleratingPositions.push_back(curPoint);
	}
	//2.b. Finish the current circle.
	while (angle < 2 * M_PI) {
		//Compute curPoint according to angle
		glm::vec3 curPoint(	   cosf(angle)*startPoint[0]-sinf(angle)*startPoint[1]
							,  sinf(angle)*startPoint[0]+cosf(angle)*startPoint[1]
							,  startPoint[2]);
		//Update cur_omega and angle
		angle += min_omega*dt; 
		//Add position
		deceleratingPositions.push_back(curPoint);
	}
	//3. Compute the acceleration path.
	// This is the inverse of the deceleration path, ordered front to back and flipping Y coords.
	for (int i = 0; i < deceleratingPositions.size()-1; i++) {
		glm::vec3 pos = deceleratingPositions[deceleratingPositions.size() - 1 - i];
		pos.y *= -1;
		acceleratingPositions.push_back(pos);
	}
	//4. Copy all generated data to output buffers:
	*circlePath_data = new float[4 * circlePositions.size()];
	*sizeCirclePath = circlePositions.size();
	for (int i = 0; i < circlePositions.size(); i++) {
		(*circlePath_data)[4 * i + 0] = circlePositions[i].x;
		(*circlePath_data)[4 * i + 1] = circlePositions[i].y;
		(*circlePath_data)[4 * i + 2] = circlePositions[i].z;
		(*circlePath_data)[4 * i + 3] = 1;
	}
	*accelPath_data = new float[4 * acceleratingPositions.size()];
	*sizeAccelPath = acceleratingPositions.size();
	for (int i = 0; i < acceleratingPositions.size(); i++) {
		(*accelPath_data)[4 * i + 0] = acceleratingPositions[i].x;
		(*accelPath_data)[4 * i + 1] = acceleratingPositions[i].y;
		(*accelPath_data)[4 * i + 2] = acceleratingPositions[i].z;
		(*accelPath_data)[4 * i + 3] = 1;
	}
	*decelPath_data = new float[4 *deceleratingPositions.size()];
	*sizeDecelPath = deceleratingPositions.size();
	for (int i = 0; i < deceleratingPositions.size(); i++) {
		(*decelPath_data)[4 * i + 0] = deceleratingPositions[i].x;
		(*decelPath_data)[4 * i + 1] = deceleratingPositions[i].y;
		(*decelPath_data)[4 * i + 2] = deceleratingPositions[i].z;
		(*decelPath_data)[4 * i + 3] = 1;
	}
	return closest_omega;
}


