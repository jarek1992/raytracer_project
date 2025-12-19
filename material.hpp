#pragma once

#include "hittable.hpp"
#include "texture.hpp"

//abstract class material
class material {
public:
	virtual ~material() = default;

	//   @brief Scatter the incoming ray according to the material's properties.
	//   
	//   @param r_in The incoming ray.
	//   @param rec The hit record containing information about the hit point.
	//   @param attenuation The color attenuation of the material.
	//   @param scattered The scattered ray.
	//   @return true if the ray is scattered, false otherwise.

	virtual bool scatter(
		const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
	) const = 0;
};

//class for lambertian material
class lambertian : public material {
public:
	//constructor for solid color albedo
	lambertian(const color& albedo) 
		: tex(make_shared<solid_color>(albedo))
	{}
	//constructor for texture albedo
	lambertian(shared_ptr<texture>tex)
		: tex(tex)
	{}

	bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
		auto scatter_direction = rec.normal + random_unit_vector();

		//catch degenerate scatter direction
		if (scatter_direction.near_zero()) {
			scatter_direction = rec.normal;
		}
		//adjust hit point by a small epsilon to avoid self-intersection
		point3 origin_adjusted = rec.p + rec.normal * ray_epsilon;

		scattered = ray(origin_adjusted, scatter_direction, r_in.time());
		//get albedo from texture
		attenuation = tex->value(rec.u, rec.v, rec.p);

		return true;
	}

private:
	shared_ptr<texture> tex;
};

//class for metal material with reflections
class metal : public material {
public:
	metal(const color& albedo, double fuzz)
		: albedo(albedo)
		, fuzz(fuzz < 1 ? fuzz : 1) //condition for fuzziness 
	{}

	bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
		vec3 reflected = reflect(r_in.direction(), rec.normal);
		reflected = unit_vector(reflected) + (fuzz * random_unit_vector());

		point3 origin_adjusted = rec.p + ray_epsilon * rec.normal;

		scattered = ray(rec.p, reflected, r_in.time());
		attenuation = albedo;
		return (dot(scattered.direction(), rec.normal) > 0);
	}

private:
	color albedo;
	double fuzz;
};

//class for dielectric material always refracts
class dielectric : public material {
public:
	dielectric(double refraction_index) : refraction_index(refraction_index) {}

	bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
		attenuation = color(1.0, 1.0, 1.0); //always white, because translucent doesnt change color
		double ri = rec.front_face ? (1.0 / refraction_index) : refraction_index; //calculation of the refractive index

		vec3 unit_direction = unit_vector(r_in.direction()); //the unit direction vector of the incoming ray.
		double cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0); //cos of the angle of incidence
		double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta); //sin of the angle of incidence(Piragoras)

		bool cannot_refract = ri * sin_theta > 1.0; //total internal reflection
		vec3 direction;

		//direction of the angle of incidence
		if (cannot_refract || reflectance(cos_theta, ri) > random_double()) {
			direction = reflect(unit_direction, rec.normal);
		}
		else {
			direction = refract(unit_direction, rec.normal, ri);
		}

		point3 origin_adjusted = rec.p + ray_epsilon * rec.normal;

		scattered = ray(rec.p, direction, r_in.time()); //creates new scattered ray with beginning = rec.p and refract direction = refracted 
		return true;
	}
private:
	//refractive index - n
	double refraction_index;
	//Schlick's approximation for reflectance
	static double reflectance(double cosine, double refraction_index) {
		auto r0 = (1 - refraction_index) / (1 + refraction_index);
		r0 = r0 * r0;
		return r0 + (1 - r0) * std::pow((1 - cosine), 5);
	}
};