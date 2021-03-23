/*
    Water Wave Simulation
    @author sanket.s.joshi.gr@dartmouth.edu
    CS 177 - Computer Graphics
    Demo : https://www.shadertoy.com/view/tlVSDK
*/

/*
    CONTROLS:
    [Mouse Click / Drag] : Pan camera
    [1] : Zoom = 2x
    [2] : Zoom = 4x
    [3] : Zoom = 8x
    [4] : Use cubemap texture for sky. 
    [5] : Use cubemap texture for underwater. 
    [6] : Reflectivity = 0.3
    [7] : Reflectivity = 0.5
    [8] : Reflectivity = 0.8
*/

/*
    TEX_SKY: 

    Whether to use a cubemap texture for the sky.
    Alternatively, a sample sky will be generated with orbiting sun and moon
*/
bool TEX_SKY = false;

/*
    TEX_GROUND: 

    Whether to use a cubemap texture for the groud under water.
    Alternatively, a fixed color will be used for the water body.
*/
bool TEX_GROUND = true;

/*
    REFLECTIVITY: 

    Value between 0 and 1, defines the reflectivity of water. 
    The refractivity value complements this.
*/
float REFLECTIVITY = 0.7;

/*
    ZOOM: 

    Value between 0 and 1, defines the reflectivity of water. 
    The refractivity value complements this.
*/
float ZOOM = 1.3;


/* DO NOT EDIT */

#define KEY_1 49.0
#define KEY_2 50.0
#define KEY_3 51.0
#define KEY_4 52.0
#define KEY_5 53.0
#define KEY_6 54.0
#define KEY_7 55.0
#define KEY_8 56.0

#define PI 3.142
#define EPSILON 1e-2
#define FLOAT_MAX 1e10
#define REFRACTIVE_INDEX 1.2

#define UP vec3(0,1,0)
#define DEPTH 2.0


struct ray1 {
    vec3 origin;
    vec3 direction;
};

struct plane {
    vec3 point;
    vec3 normal;
};

bool isPressed(float keyCode) {

    // Reference: https://www.shadertoy.com/view/XlGfzt
    
    keyCode = (keyCode + 0.5) / 256.0;
    vec2 uv = vec2(keyCode, 0.25);
    float key = texture(iChannel2, uv).r;
    return key > 0.0;
}

bool isToggled(float keyCode) {
    
    // Reference: https://www.shadertoy.com/view/XlGfzt
    
    keyCode = (keyCode + 0.5) / 256.0;
    vec2 uv = vec2(keyCode, 0.75);
    float key = texture(iChannel2, uv).r;
    return key > 0.0;
}

void setKeyBindings() {

    if (isPressed(KEY_1))
        ZOOM = 2.0;

    if (isPressed(KEY_2))
        ZOOM = 4.0;

    if (isPressed(KEY_3))
        ZOOM = 8.0;

    if (isToggled(KEY_4))
        TEX_SKY = !TEX_SKY;

    if (isToggled(KEY_5))
        TEX_GROUND = !TEX_GROUND;

    if (isPressed(KEY_6))
        REFLECTIVITY = 0.3;

    if (isPressed(KEY_7))
        REFLECTIVITY = 0.5;

    if (isPressed(KEY_8))
        REFLECTIVITY = 0.8;
}

vec3 travel(ray1 r, float dist) {
    return r.origin + r.direction * dist;
}

vec3 getRayDirection(vec2 uv) {

    // Calculate camera angle
    
    float camAngleHorz = (2.0 * iMouse.x / iResolution.x - 1.0) * PI;
    float camAngleVert = (2.0 * iMouse.y / iResolution.y - 1.0) * PI / 2.0;

    float sv = sin(camAngleVert);
    float cv = cos(camAngleVert);
    float sh = sin(camAngleHorz);
    float ch = cos(camAngleHorz);

    mat3 camDirRot = mat3(
        ch, 0, sh,
        -sh * sv, cv, ch * sv,
        -sh * cv, -sv, ch * cv
    );

    // Calculate pixel angle
    
    float aspectRatio = iResolution.x / iResolution.y;
    float pixelAngleHorz = (2.0 * uv.x / iResolution.x - 1.0) * aspectRatio;
    float pixelAngleVert = (2.0 * uv.y / iResolution.y - 1.0);
    vec3 pixDir = vec3(pixelAngleHorz, pixelAngleVert, ZOOM);

    return normalize(camDirRot * pixDir);
}

vec3 intersect(ray1 ray, plane p) {

    float dist = dot(p.point - ray.origin, p.normal) / dot(ray.direction, p.normal);
    return travel(ray, clamp(dist, -1.0, FLOAT_MAX));
}

vec4 getSampleSky(vec3 dir) {

    float c = cos(iTime / 2.0);
    float s = sin(iTime / 2.0);
    vec3 light_loc = normalize(vec3(s, 0.7 * c, c));
    
    // Simulate sun / moon
    if (abs(dot(light_loc, normalize(dir))) > 0.995)
        return vec4(FLOAT_MAX);
    
    // Default sky value
    float angle = pow(abs(asin(normalize(dir).y)*2.0/PI),0.5);
    vec3 color = mix(vec3(1),vec3(0.6, 0.8, 1),angle);
    return vec4(color,1) * (1.5 + c) / 2.5;
}

float wave(vec2 pos, int iterations) {
    
    // Initialize frequencies
    float spacial_angle = 0.0;
    float spacial_frequency = PI * 2.0;
    float temporal_frequency = PI / 2.0;
    
    // Initialize height weights
    float w = 1.0; // Running weight
    float h = 0.0; // Weighted height
    float ht = 0.0; // Sum of weighted heights
    pos *= 0.1;

    for (int i = 0; i < iterations; i++) {
        
        // Calculate wave travel direction
        vec2 dir = vec2(sin(spacial_angle), cos(spacial_angle));
        float t = spacial_frequency * dot(dir, pos) + temporal_frequency * iTime;
        
        // Calculate current frequency's height
        float height = exp(sin(t) - 1.0);
        float height_d = -exp(sin(t) - 1.0) * cos(t);
        
        // Add to weighted heights
        h += height * w;
        ht += w;
        
        // Adjust spacial angle and position
        spacial_angle += 10.0;
        pos += dir * height_d * w * 0.1;
        
        // Adjust frequencirs nad running weight
        spacial_frequency *= 1.21;
        temporal_frequency *= 1.06;
        w *= 0.78;
    }
    
    // Return net weighted height across all iterations
    return h / ht;
}

float rayMarch(ray1 ray, vec3 start, vec3 end) {
    
    vec3 current = start;
    for (int i = 0; i < 300; i++) {
        // SDF calculation
        float y = DEPTH * (wave(current.xz, 10) - 1.0);
        if (current.y - y < EPSILON)
            return distance(current, ray.origin);
        // Traverse step
        current += ray.direction * (current.y - y);
    }
    return 0.0;
}

vec3 perpendicular(vec3 v1, vec3 v2) {
    return normalize(cross(normalize(v1), normalize(v2)));
}

vec3 normal(vec2 pos) {
    
    // Depths around neighbourhood
    float depth0 = wave(pos, 50);
    float depth1 = wave(pos - vec2(EPSILON, 0), 50);
    float depth2 = wave(pos + vec2(0, EPSILON), 50);
    
    // Cross product of orthogonal vicinity differences
    return perpendicular(
        vec3(EPSILON / DEPTH, depth0 - depth1, 0),
        vec3(0, depth0 - depth2, -EPSILON / DEPTH)
    );
}

vec3 calculateNormal(ray1 ray, plane water_ceiling, plane water_floor) {

    // Find upper and lower bounds for hits
    vec3 hit_ceil = intersect(ray, water_ceiling);
    vec3 hit_floor = intersect(ray, water_floor);
    float hit_dist = rayMarch(ray, hit_ceil, hit_floor);
    vec3 hit_pos = travel(ray, hit_dist);
    vec3 normal = normal(hit_pos.xz);

    if (TEX_SKY) {
        // Adjust for close distance reflection
        float mix_extent = EPSILON * pow(hit_dist, 2.0);
        normal = (normal + UP * mix_extent) / (1.0 + mix_extent);
    }

    return normal;
}

vec4 calculateReflected(vec3 incident, vec3 reflected, vec3 normal) {

    if (TEX_SKY)
        return texture(iChannel0, reflected);
    
    // Incorporate diffraction
    float scatter_sharpness = 7.0;
    float scatter_offset = 0.4;
    float scatter = 1.0 - max(0.0, dot(-normal, incident));
    scatter = scatter_offset + pow(scatter, scatter_sharpness);

    return getSampleSky(reflected) * scatter;
}

vec4 calculateRefracted(vec3 refracted, vec3 normal) {

    if (TEX_GROUND)
        return texture(iChannel1, refracted);

    if (TEX_SKY)
        return vec4(0.1, 0.1, 0.1, 1.0);

    return vec4(0.15, 0.2, 0.25, 1.0);
}

vec4 getSkyFragColor(ray1 ray) {

    if (TEX_SKY)
        return texture(iChannel0, ray.direction);

    return getSampleSky(ray.direction);
}

vec4 getGroundFragColor(ray1 ray) {
    
    // Define upper and lower bounds
    plane water_ceiling = plane(vec3(0), UP);
    plane water_floor = plane(vec3(0, -DEPTH, 0), UP);
    vec3 normal = calculateNormal(ray, water_ceiling, water_floor);
    
    // Calculate reflected component
    vec3 dirReflected = reflect(ray.direction, normal);
    vec4 colReflected = calculateReflected(ray.direction, dirReflected, normal);

    // Calculate refracted component
    vec3 dirRefracted = refract(ray.direction, normal, REFRACTIVE_INDEX);
    vec4 colRefracted = calculateRefracted(dirRefracted, normal);
    
    // Weighted sum based on reflectivity
    return mix(colRefracted, colReflected, REFLECTIVITY);
}

vec4 getColor(ray1 ray) {
    
    // Towards the sky
    if (ray.direction.y >= -EPSILON)
        return getSkyFragColor(ray);
    
    // Towards the water
    return getGroundFragColor(ray);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    
    // Initialize key bindings
    setKeyBindings();

    // Set camera based on depth
    vec3 cam = DEPTH * UP;

    // Get ray based on pixel and mouse location
    ray1 ray = ray1(cam, getRayDirection(fragCoord));

    // Calculate color for the ray
    fragColor = getColor(ray);
}

