#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "Cache/Plugins/xgeom_static.plugin/source/runtime/mb_standard_pbr.frag"
#include "Cache/Plugins/xgeom_static.plugin/source/runtime/mb_tone_mapper_lion.frag"
#include "Cache/Plugins/xgeom_static.plugin/source/runtime/mb_lineartogamma.frag"

//layout(binding = 0) uniform sampler2D SamplerShadowMap;        // [INPUT_TEXTURE_100]	// depth system dependent
layout(binding = 1) uniform sampler2D SamplerNormal;           // [INPUT_TEXTURE_110]	// liniar BC5 Tangent space normal
layout(binding = 2) uniform sampler2D SamplerAlbedo;           // [INPUT_TEXTURE_120]	// SRGB decompress by Vulkan, BC1/BC7 albedo
layout(binding = 3) uniform sampler2D SamplerORME;             // [INPUT_TEXTURE_130]	// liniar BC1/BC7 ORM packing (AO in R, Roughness in G, Metalness in B)

layout(location = 0) out vec4 outFragColor;

vec3 getBC5Normal( vec2 UV )
{
	vec3 Normal;
	Normal.xy = (texture(SamplerNormal, UV).gr * 2.0) - 1.0;
	Normal.z = sqrt(1.0 - dot(Normal.xy, Normal.xy));
	return Normal;
}

void main()
{
	vec4 ORME		= texture(SamplerORME, In.UV);
	vec4 Albedo		= texture(SamplerAlbedo, In.UV);
	vec3 FinalColor = PBRLighting
	( getBC5Normal(In.UV).xyz
	, Albedo.rgb
	, ORME.r
	, 0.04f
	, ORME.g
	, ORME.b
	, ORME.a * Albedo.rgb
	);

	FinalColor = ToneMapper_lion(FinalColor);

	outFragColor.a = 1;
	outFragColor.rgb = linearToSrgb(FinalColor);
}