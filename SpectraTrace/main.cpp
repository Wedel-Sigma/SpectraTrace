#include <fstream>
#include <iostream>
#include <cmath>
#include <stdexcept>

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

int main() {
	/*std::ofstream file("output.ppm");
	int width = 100;
	int height = 100;


	if (file.is_open()) {
		file << "P3\n" <<width << " " << height<< "\n255\n";
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				file << x << " " << y << " 0 ";
			}
			file << "\n";
		}
	}
	else {
		std::cout << "Nie udalo sie otworzyc pliku";
	}*/
	Vec3 dir = normalize({ 2,1,0 });
	Vec3 normal = { 0,-1,0 };

	Vec3 res = refract(dir, normal, 1.3333, 1.0);

	std::cout << res.x << ", " << res.y << ", " << res.z;
}