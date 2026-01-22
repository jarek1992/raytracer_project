#pragma once

#include "hittable.hpp"
#include "texture.hpp"

//abstract class material
class material {
public:
	virtual ~material() = default;

	//no emission by default black color material
	virtual color emitted(double u, double v, const point3& p) const {
		return color(0, 0, 0); //default no emission
	}

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

	//denoising function
	virtual color get_albedo(const hit_record& rec) const {
		return color(0, 0, 0); //default black
	}

protected:
	//function to modify normal in hit_record 
	vec3 get_bumped_normal(const hit_record& rec, shared_ptr<texture> bump_map, double strength) const {
		if (!bump_map) {
			return rec.normal; //no bump map provided
		}

		double du = 1.0 / 1024.0; //small offset in u direction
		double dv = 1.0 / 1024.0; //small offset in v direction

		//get brightness values from bump map texture (black-white map, get R channel)
		double height_center = bump_map->value(rec.u, rec.v, rec.p).x();
		double height_u = bump_map->value(rec.u + du, rec.v, rec.p).x();
		double height_v = bump_map->value(rec.u, rec.v + dv, rec.p).x();

		double f_u = (height_u  - height_center) * strength; //height difference in u direction
		double f_v = (height_v  - height_center) * strength; //height difference in v direction

		//N' = N - f_u * Tangent - f_v * Bitangent
		vec3 bumped_normal = rec.normal - (f_u * rec.tangent) - (f_v * rec.bitangent);
		return unit_vector(bumped_normal); //normalize the new normal
	}
};

//class for lambertian material
class lambertian : public material {
public:
	//constructor for solid color albedo
	lambertian(const color& albedo, shared_ptr<texture> bump = nullptr, double strength = 1.0)
		: tex(make_shared<solid_color>(albedo))
		, my_bump_texture(bump)
		, bump_strength(strength)
	{}
	//constructor for texture albedo
	lambertian(shared_ptr<texture>tex, shared_ptr<texture> bump = nullptr, double strength = 1.0)
		: tex(tex)
		, my_bump_texture(bump)
		, bump_strength(strength)
	{}

	bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
		//get normal from hit record
		vec3 working_normal = rec.normal;
		//condition for bump mapping
		if (my_bump_texture) {
			working_normal = get_bumped_normal(rec, my_bump_texture, bump_strength);
		}

		auto scatter_direction = working_normal + random_unit_vector();

		//catch degenerate scatter direction
		if (scatter_direction.near_zero()) {
			scatter_direction = working_normal;
		}
		//adjust hit point by a small epsilon to avoid self-intersection
		point3 origin_adjusted = rec.p + (rec.normal * ray_epsilon);

		scattered = ray(origin_adjusted, scatter_direction, r_in.time());
		//get albedo from texture
		attenuation = tex->value(rec.u, rec.v, rec.p);

		return true;
	}

	//overwrite OIDN function
	color get_albedo(const hit_record& rec) const override {
		//return raw texture color in hit point
		return tex->value(rec.u, rec.v, rec.p);
	}

private:
	shared_ptr<texture> tex;
	shared_ptr<texture> my_bump_texture; //bump map texture pointer
	double bump_strength; //bump map strength
};

//class for metal material with reflections
class metal : public material {
public:
	//constructor for checker texture
	metal(shared_ptr<texture> a, double f, shared_ptr<texture> bump = nullptr, double strength = 1.0)
		: albedo(a)
		, fuzz(f < 1 ? f : 1) //condition for fuzziness 
		, my_bump_texture(bump)
		, bump_strength(strength)
	{}

	//constructor for solid color albedo(user friendly)
	metal(const color& a, double f, shared_ptr<texture> bump = nullptr, double strength = 1.0)
		: albedo(make_shared<solid_color>(a))
		, fuzz(f < 1 ? f : 1) //condition for fuzziness 
		, my_bump_texture(bump)
		, bump_strength(strength)
	{}

	bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
		//get normal from hit record
		vec3 working_normal = rec.normal;
		//condition for bump mapping
		if (my_bump_texture) {
			working_normal = get_bumped_normal(rec, my_bump_texture, bump_strength);
		}
		//mirror-like reflection
		vec3 v = unit_vector(r_in.direction());
		vec3 reflected = reflect(v, working_normal);

		//adding fuzziness to the reflection
		vec3 scattered_direction = unit_vector(reflected + (fuzz * random_unit_vector()));

		//moving the ray origin a bit off the surface to prevent self-intersection (shadow acne)
		point3 shadow_orig = rec.p + (ray_epsilon * rec.normal);

		scattered = ray(shadow_orig, scattered_direction, r_in.time());
		attenuation = albedo->value(rec.u, rec.v, rec.p);

		//if the scattered ray is in the same hemisphere as the normal
		return (dot(scattered.direction(), rec.normal) > 0);
	}

	//albedo function for denoiser
	color get_albedo(const hit_record& rec) const override {
		return albedo->value(rec.u, rec.v, rec.p);
	}

private:
	shared_ptr<texture> albedo;
	double fuzz;
	shared_ptr<texture> my_bump_texture; //bump map texture pointer
	double bump_strength; //bump map strength
};

//class for dielectric material always refracts
class dielectric : public material {
public:
	dielectric(double refraction_index, const color& a = color(1.0, 1.0, 1.0), shared_ptr<texture> bump = nullptr, double strength = 1.0)
		: refraction_index(refraction_index)
		, albedo(a)
		, my_bump_texture(bump)
		, bump_strength(strength)
	{}

	bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
		attenuation = albedo; //adjust color for glass
		//get normal from hit record
		vec3 working_normal = rec.normal;
		if (my_bump_texture) {
			working_normal = get_bumped_normal(rec, my_bump_texture, bump_strength); //for dielectric less strenth (1.0 normally is enough)
		}

		double ri = rec.front_face ? (1.0 / refraction_index) : refraction_index; //calculation of the refractive index

		vec3 unit_direction = unit_vector(r_in.direction()); //the unit direction vector of the incoming ray.

		double cos_theta = std::fmin(dot(-unit_direction, working_normal), 1.0); //cos of the angle of incidence
		double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta); //sin of the angle of incidence(Piragoras)

		bool cannot_refract = ri * sin_theta > 1.0; //total internal reflection
		vec3 direction;

		//direction of the angle of incidence
		if (cannot_refract || reflectance(cos_theta, ri) > random_double()) {
			direction = reflect(unit_direction, working_normal);
		}
		else {
			direction = refract(unit_direction, working_normal, ri);
		}

		//shadow acne (translation origin point)
		//move the ray origin a bit off the surface to prevent self-intersection
		vec3 offset = (dot(direction, rec.normal) > 0) ? (ray_epsilon * rec.normal) : (-ray_epsilon * rec.normal);
		point3 origin_adjusted = rec.p + offset;

		scattered = ray(origin_adjusted, direction, r_in.time()); //creates new scattered ray with beginning = rec.p and refract direction = refracted 
		return true;
	}

	color get_albedo(const hit_record& rec) const override {
		//for glas return (1,1,1), ODIN let the ray goes through 
		return color(1.0, 1.0, 1.0);
	}

private:
	double refraction_index; //refractive index - n
	color albedo; //color of the material
	shared_ptr<texture> my_bump_texture; //bump map texture pointer
	double bump_strength; //bump map strength
	//Schlick's approximation for reflectance
	static double reflectance(double cosine, double refraction_index) {
		auto r0 = (1 - refraction_index) / (1 + refraction_index);
		r0 = r0 * r0;
		return r0 + (1 - r0) * std::pow((1 - cosine), 5);
	}
};

//class for diffuse light-emitting material
class diffuse_light : public material {
public:
	diffuse_light(shared_ptr<texture> a)
		: emit(a)
	{}
	diffuse_light(color c)
		: emit(make_shared<solid_color>(c))
	{}

	bool scatter(
		const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
	) const override {
		return false; //no scattering for light-emitting materials
	}
	color emitted(double u, double v, const point3& p) const {
		return emit->value(u, v, p);
	}

	//denoising fucntion OIDN
	color get_albedo(const hit_record& rec) const override {
		//get texture/light color
		color c = emit->value(rec.u, rec.v, rec.p);
		//limit max to 1.0
		return color(
			std::fmin(c.x(), 1.0),
			std::fmin(c.y(), 1.0),
			std::fmin(c.z(), 1.0)
		);
	}

private:
	shared_ptr<texture> emit;
};