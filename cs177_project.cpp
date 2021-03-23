// Demo at https://www.shadertoy.com/view/tlVSDK

// Variables to be edited during demo

#define REFLECTIVITY 0.6
#define TEX_ABOVE true
#define TEX_BELOW true

// Other variables

#define PI 3.142
#define EPSILON 1e-2
#define FLOAT_MAX 1e10
#define REFRACTIVE_INDEX 1.1

#define KEY_1 49.
#define KEY_2 50.
#define KEY_3 51.

#define DRAG_MULT 0.048
#define ITERATIONS_RAYMARCH 10
#define ITERATIONS_NORMAL 40
#define DEPTH 2.

float zoom = 1.3;
vec3 up = vec3(0,1,0);

struct ray {
    vec3 origin;
    vec3 direction;
};

struct plane {
    vec3 point;
    vec3 normal;
};


/*
    Credits: https://www.shadertoy.com/view/XlGfzt
*/

bool isPressed(float keyCode) {
    keyCode = (keyCode + 0.5) / 256.0;
    vec2 uv = vec2(keyCode, 0.25);
    float key = texture(iChannel2, uv).r;
    return key > 0.0;
}

/*
    Credits: https://www.shadertoy.com/view/XlGfzt
*/
bool isToggled(float keyCode) {
    keyCode = (keyCode + 0.5) / 256.0;
    vec2 uv = vec2(keyCode, 0.75);
    float key = texture(iChannel2, uv).r;
    return key > 0.0;
}

vec2 wavedx(vec2 position, vec2 direction, float speed, float frequency, float timeshift) {
    float x = dot(direction, position) * frequency + timeshift * speed;
    float wave = exp(sin(x) - 1.0);
    float dx = wave * cos(x);
    return vec2(wave, -dx);
}

float getwaves(vec2 position, int iterations) {
    float iter = 0.0;
    float phase = 6.0;
    float speed = 2.0;
    float weight = 1.0;
    float w = 0.0;
    float ws = 0.0;
    for (int i = 0; i < iterations; i++) {
        vec2 p = vec2(sin(iter), cos(iter));
        vec2 res = wavedx(position, p, speed, phase, iTime);
        position += normalize(p) * res.y * weight * DRAG_MULT;
        w += res.x * weight;
        iter += 12.0;
        ws += weight;
        weight = mix(weight, 0.0, 0.2);
        phase *= 1.18;
        speed *= 1.07;
    }
    return w / ws;
}

float rayMarch(ray ray, vec3 start, vec3 end) {

    float h = 0.0;
    vec3 pos = start;
    for (int i = 0; i < 400; i++) {
        h = getwaves(pos.xz * 0.1, ITERATIONS_RAYMARCH) * DEPTH - DEPTH;
        if (h + EPSILON > pos.y) 
            return distance(pos, ray.origin);
        pos += ray.direction * (pos.y - h);
    }
    return 0.;
}

vec3 perpendicular(vec3 v1, vec3 v2) {
    return normalize(cross(normalize(v1), normalize(v2)));
}

vec3 normal(vec2 pos) {

    vec2 ex = vec2(EPSILON, 0);
    float del = 0.1;
    float H = getwaves(pos.xy * del, ITERATIONS_NORMAL) * DEPTH;
    float depth1 = getwaves(pos.xy * del - ex.xy * del, ITERATIONS_NORMAL) * DEPTH;
    float depth2 = getwaves(pos.xy * del + ex.yx * del, ITERATIONS_NORMAL) * DEPTH;

    return perpendicular(
        vec3(EPSILON, H-depth1, 0),
        vec3(0, H-depth2, -EPSILON)
    );
}

vec3 getRayDirection(vec2 uv) {

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
    
    float aspectRatio = iResolution.x / iResolution.y;
    float pixelAngleHorz = (2.0 * uv.x / iResolution.x - 1.0) * aspectRatio;
    float pixelAngleVert = (2.0 * uv.y / iResolution.y - 1.0);
    vec3 pixDir = vec3(pixelAngleHorz, pixelAngleVert, zoom);

    return normalize(camDirRot * pixDir);
}

vec3 intersect(ray r, plane p) {

    float dist = dot(p.point - r.origin, p.normal) / dot(r.direction, p.normal);
    return r.origin + r.direction * clamp(dist, -1.0, FLOAT_MAX);
}

vec4 getSampleSky(vec3 dir) {
    
    float c = cos(iTime);
    float s = sin(iTime);
    vec3 light_loc = normalize(vec3(s,0.7*c,c));
    if (abs(dot(light_loc,normalize(dir)))>0.99)
        return vec4(10);
    return vec4(0.6,0.8,1,1) * (1.5 + c)/2.5;
}

void handleKeyEvents() {

    if (isPressed(KEY_1))
        zoom *= 2.0;
    if (isPressed(KEY_2))
        zoom *= 4.0;
    if (isPressed(KEY_3))
        zoom *= 8.0;
}

vec3 calculateNormal(ray ray, plane water_ceiling, plane water_floor) {

    vec3 hit_ceil = intersect(ray, water_ceiling);
    vec3 hit_floor = intersect(ray, water_floor);
    float hit_dist = rayMarch(ray, hit_ceil, hit_floor);
    vec3 hit_pos = ray.origin + ray.direction * hit_dist;

    vec3 normal = normal(hit_pos.xz);
    if(TEX_ABOVE)
        normal = mix(up, normal, 1.0 / (hit_dist * hit_dist * EPSILON + 1.0));

    return normal;
}

vec4 calculateReflected(vec3 rayDir, vec3 reflected, vec3 normal) {

    if(TEX_ABOVE)
        return texture(iChannel0, reflected);
    return getSampleSky(reflected) * (0.4 + (1.0-0.04)*(pow(1.0 - max(0.0, dot(-normal, rayDir)), 5.0)));;
}

vec4 calculateRefracted(vec3 refracted, vec3 normal) {
    
    if(TEX_BELOW)
        return texture(iChannel1, refracted);
    if(TEX_ABOVE)
        return vec4(0.1,0.1,0.1,1);
    return vec4(0.15,0.2,0.25,1);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {

    handleKeyEvents();

    vec3 cam = vec3(0, 2. * DEPTH, 0);

    plane water_ceiling = plane(vec3(0),up);
    plane water_floor = plane(vec3(0,-DEPTH,0),up);

    ray ray = ray(cam, getRayDirection(fragCoord));

    if (ray.direction.y >= -EPSILON) {
        if(TEX_ABOVE)
            fragColor = texture(iChannel0, ray.direction);
        else
            fragColor = getSampleSky(ray.direction);
        return;
    }

    vec3 normal = calculateNormal(ray, water_ceiling, water_floor);

    vec3 dirReflected = reflect(ray.direction, normal);
    vec4 colReflected = calculateReflected(ray.direction, dirReflected, normal);
    
    vec3 dirRefracted = refract(ray.direction, normal, REFRACTIVE_INDEX);
    vec4 colRefracted = calculateRefracted(dirRefracted, normal);

    fragColor = colReflected * REFLECTIVITY + colRefracted * (1.-REFLECTIVITY);
}
