#include <fstream>
#include <iostream>
#include <cmath>
#include <stdexcept>
#include <numbers>
#include <algorithm>

struct Vec3 {
	double x;
	double y;
	double z;
};

struct Ray {
	Vec3 origin;
	Vec3 direction;
};

Vec3 add(Vec3 a, Vec3 b) {
	Vec3 sum;

	sum.x = a.x + b.x;
	sum.y = a.y + b.y;
	sum.z = a.z + b.z;

	return sum;
}

Vec3 multiply(Vec3 a, double scalar) {
	return { a.x * scalar, a.y * scalar, a.z * scalar };
}

Vec3 subtract(Vec3 a, Vec3 b) {
	return{ a.x - b.x, a.y - b.y, a.z - b.z };
}

double length(Vec3 v) {
	return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vec3 pointAt(Ray ray, double t) {
	return add(ray.origin, multiply(ray.direction, t));
}

Vec3 normalize(Vec3 v) {
	double lengthV = length(v);
	if (lengthV == 0) {
		throw std::invalid_argument("cannot normalize zero vector");
	}

	return { v.x / lengthV, v.y / lengthV, v.z / lengthV };
}

double dot(Vec3 a, Vec3 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 reflect(Vec3 direction, Vec3 normal) {
	return subtract(direction,multiply(normal, 2 * dot(direction, normal)));
}

double refractionRatio(double n1, double n2) {
	if (n2 <= 0) { throw std::invalid_argument("refractive index must be positive"); }
	return n1 / n2;
}

double incidenceCosine(Vec3 direction, Vec3 normal) {
	return -dot(direction, normal);
}

double refractionDiscriminant(double eta, double c) {
	return 1 - eta * eta * (1 - c * c);
}

Vec3 refract(Vec3 direction, Vec3 normal, double n1, double n2) {
	double eta = refractionRatio(n1, n2);
	double c = incidenceCosine(direction, normal);
	double k = refractionDiscriminant(eta, c);
	if (k < 0) {
		return reflect(direction, normal);
	}

	return add(multiply(direction, eta), multiply(normal, eta * c - std::sqrt(k)));
}

double intersectWaterPlane(Ray ray, double waterHeight) {
	if (ray.direction.y == 0) return -1;
	double t = ( waterHeight - ray.origin.y ) / ray.direction.y;
	if (t > 0) { return t; }
	return -1;
}

double pixelToScreenX(int x, int width) {
	return 2.0 * (x + 0.5) / width - 1.0;
}

double pixelToScreenY(int y, int height) {
	return 1.0 - 2.0 * (y + 0.5) / height;
}

Vec3 pixelDirection(int x, int y, int width, int height, double fovDegrees) {
	double ar = double(width) / height;
	double screenX = pixelToScreenX(x, width) * ar;

	double scale = std::tan(fovDegrees * (std::numbers::pi / 180) / 2);

	return normalize({ screenX * scale, pixelToScreenY(y, height) * scale, -1 });
}

Vec3 pointOnStick(Vec3 start, Vec3 end, double u) {
	return add(start, multiply(subtract(end, start), u));
}

double distanceSquared(Vec3 a, Vec3 b) {
	Vec3 diff = subtract(a, b);
	return dot(diff, diff);
}

double stickProjection(Vec3 point, Vec3 start, Vec3 end) {
	Vec3 diff = subtract(end, start);
	return dot(subtract(point, start),diff) / dot(diff, diff);
}

Vec3 closestPointOnStick(Vec3 point, Vec3 start, Vec3 end) {
	double u = std::clamp(stickProjection(point, start, end), 0.0, 1.0);
	return pointOnStick(start, end, u);
}

double distanceSquaredToStick(Vec3 point, Vec3 start, Vec3 end) {
	return distanceSquared(
		point,
		closestPointOnStick(point, start, end)
	);
}

int main() {
	std::ofstream file("output.ppm");
	int width = 1920;
	int height = 1080;
	double FOV = 60;
	double waterHeight = 0;
	double bottomHeight = -2;

	Vec3 stickStart = { 1,1,-5 };
	Vec3 stickEnd = { 0,-1,-5 };
	double stickRadius = 0.1;

	if (file.is_open()) {
		file << "P3\n" <<width << " " << height<< "\n255\n";
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				Ray ray = { { 0,2,0 },pixelDirection(x, y, width, height, FOV) };
				double t = intersectWaterPlane(ray, waterHeight);
				
				if (t > 0) {
					Ray refractedRay = { pointAt(ray, t),refract(ray.direction,{ 0,1,0 },1.0, 1.3333) };

					double bottomT = intersectWaterPlane(refractedRay, bottomHeight);
					if (bottomT > 0) {
						Vec3 bottomPoint = pointAt(refractedRay, bottomT);

						int cellX = int(std::floor(bottomPoint.x));
						int cellZ = int(std::floor(bottomPoint.z));

						if ((cellX + cellZ) % 2 == 0) {
							file << "160 230 255 ";
						}
						else {
							file << "0 70 95 ";
						}
					}
					else {
						file << "0 188 255 ";
					}
					
				}
				else {
					file << "135 206 245 ";
				}
				
			}
			file << "\n";
		}
	}
	else {
		std::cout << "Nie udalo sie otworzyc pliku";
	}
}