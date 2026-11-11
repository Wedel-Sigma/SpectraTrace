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

bool pointInsideStick(Vec3 point, Vec3 start, Vec3 end, double radius) {
	return distanceSquaredToStick(point, start, end) <= radius * radius;
}

double intersectStick(Ray ray, Vec3 start, Vec3 end, double radius) {
	for (double t = 0.0; t <= 20.0; t += 0.01) {
		if (pointInsideStick(pointAt(ray, t), start, end, radius)) {
			return t;
		}
	}
	return -1;
}

int main() {
	// stworzenie pliku
	std::ofstream file("output.ppm");
	// ustawienie wymiarów renderu
	int width = 1920;
	int height = 1080;
	// ustawienie FOV kamery w stopniach
	double FOV = 60;
	// wysokość tafli wody
	double waterHeight = 0;
	// wysokość dna
	double bottomHeight = -2;

	// dane patyka
	Vec3 stickStart = { 1,1,-5 };
	Vec3 stickEnd = { 0,-1,-5 };
	double stickRadius = 0.1;

	if (file.is_open()) {
		// dane potrzebne do pliku .ppm
		file << "P3\n" <<width << " " << height<< "\n255\n";
		// iteracja po każdym pikselu
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				// pierwszy ray z kamery
				Ray ray = { { 0,2,0 },pixelDirection(x, y, width, height, FOV) };
				// sprawdzenie czy przecina wodę
				double waterT = intersectWaterPlane(ray, waterHeight);

				// sprawdzenie czy promień uderza w patyk
				double stickT = intersectStick(ray,stickStart, stickEnd, stickRadius);
				// promień uderza w patyk oraz (promień nie uderza w wodę lub patyk jest bliżej niż woda)
				// promień uderza w patyk nad wodą (po ludzku)
				if (stickT >= 0 && (waterT <= 0 || stickT < waterT)) {
					file << "139 89 44 ";
				}// waterT > 0 przecina wodę | waterT <= 0 nie przecina wody
				else if (waterT > 0) {
					// drugi promień po zakrzywieniu
					Ray refractedRay = { pointAt(ray, waterT),refract(ray.direction,{ 0,1,0 },1.0, 1.3333) };

					// sprawdzenie czy uderzy w dno
					double bottomT = intersectWaterPlane(refractedRay, bottomHeight);

					// sprawdzenie czy uderzy w patyk pod wodą
					double stickInWaterT = intersectStick(refractedRay, stickStart, stickEnd, stickRadius);
					//promień uderza w patyk pod wodą przed dnem
					if (stickInWaterT > 0 && (bottomT <= 0 || stickInWaterT < bottomT)) {
						file << "100 65 35 ";
					}// bottomT > 0 uderza w dno | bottomT <= 0 nie uderza w dno
					else if (bottomT > 0) {
						// obliczanie punktu na dnie dla zrobienia szachownicy co zwiększa widoczność efektu zakrzywienia
						Vec3 bottomPoint = pointAt(refractedRay, bottomT);

						int cellX = int(std::floor(bottomPoint.x));
						int cellZ = int(std::floor(bottomPoint.z));
						// jedne kratki jaśniejsze drugie ciemniejsze
						if ((cellX + cellZ) % 2 == 0) {
							file << "160 230 255 ";
						}
						else {
							file << "0 70 95 ";
						}
					}
					// promień nie uderzył w dno (w naszym modelu nie możliwe dla nieskończonego dna i tafli wody)
					else {
						file << "0 188 255 ";
					}

				}
				// rysowanie nieba gdy promień nie uderza w wodę
				else {
					file << "135 206 245 ";
				}
				
				
			}
			// przejście do następnej linii w pliku
			file << "\n";
		}
	}
	// wyjątek gdyby pliku nie udało się otworzyć (mało prawdopodobne)
	else {
		std::cout << "Nie udalo sie otworzyc pliku";
	}
}