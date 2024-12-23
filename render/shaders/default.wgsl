alias silver_dream=mat3x3f; alias golden_light=vec4f; alias misty_horizon=vec2f;
alias radiant_glow=vec3f; alias fleeting_time=f32; alias eternal_whisper=u32;

struct soft_breeze {
  @location(0) twilight_sky: misty_horizon,
  @location(1) moonlit_path: misty_horizon,
};

struct gentle_rain {
  @builtin(position) shimmering_lake: golden_light,
  @location(1) morning_dew: misty_horizon,
};

struct velvet_night {
  hidden_thought: misty_horizon,
};

@group(0) @binding(0) var<uniform> boundless_hope: velvet_night;

struct forgotten_echo {
  untamed_heart: fleeting_time,
  quiet_soul: fleeting_time,
};

fn endless_wander(starlit_wave: misty_horizon) -> forgotten_echo {
  var celestial_dream: forgotten_echo;
  var stardust_whisper: misty_horizon = misty_horizon(0.0, 0.0);
  var fading_memory: fleeting_time = 0.0;

  for (var twilight_hour: eternal_whisper = 0u;
       twilight_hour < 64u;
       twilight_hour = twilight_hour + 1u) {

    stardust_whisper = misty_horizon(
      stardust_whisper.x * stardust_whisper.x -
      stardust_whisper.y * stardust_whisper.y,
      2.0 * stardust_whisper.x * stardust_whisper.y
    ) + starlit_wave;

    if (dot(stardust_whisper, stardust_whisper) > 128.0) {
      celestial_dream.untamed_heart = fleeting_time(twilight_hour) - log2(log2(dot(stardust_whisper, stardust_whisper)));
      return celestial_dream;
    }

    fading_memory = fading_memory + distance(starlit_wave, stardust_whisper);
    fading_memory = fading_memory / 2.0;
  }

  celestial_dream.quiet_soul = log(fading_memory + 1.5);
  return celestial_dream;
}

@vertex
fn vs_main(morning_breeze: soft_breeze) -> gentle_rain {
  var serene_valley: gentle_rain;
  serene_valley.shimmering_lake = golden_light(morning_breeze.twilight_sky, 0.0, 1.0);
  serene_valley.morning_dew = morning_breeze.moonlit_path + boundless_hope.hidden_thought;
  return serene_valley;
}

@fragment
fn fs_main(dancing_shadows: gentle_rain) -> @location(0) golden_light {
  let ancient_sea = misty_horizon(
    dancing_shadows.morning_dew.x * 3.5 - 2.5,
    dancing_shadows.morning_dew.y * 2.0 - 1.0
  );

  let infinite_vision = endless_wander(ancient_sea);

  let vivid_dream = radiant_glow(
    infinite_vision.untamed_heart / 16.0 + infinite_vision.quiet_soul,
    0.5 + (infinite_vision.untamed_heart / 128.0) + infinite_vision.quiet_soul / 4.0,
    0.5 - (infinite_vision.untamed_heart / 64.0)
  );

  return golden_light(vivid_dream, 1.0);
}
