const display_elem = document.querySelector(".display");

const dmx_r_elem = document.querySelector('.dmx_r');
const dmx_g_elem = document.querySelector('.dmx_g');
const dmx_b_elem = document.querySelector('.dmx_b');
const dmx_dim_elem = document.querySelector('.dmx_dim');
const dmx_shutter_elem = document.querySelector('.dmx_shutter');
const dmx_freq_elem = document.querySelector('.dmx_freq');
const dmx_speed_elem = document.querySelector('.dmx_speed');

const ctx = display_elem.getContext("2d");

let n_leds = 150;
let frame_buffer = [];

// shutter variables
let shutter_interval_handler;
let shutter_closed = false;

// wave animation variables
let wave_interval_handler;
let wave_x_position = 0;

// display frame_buffer
let draw_leds = () => {
    // for each pixel
    for(let i = 0; i < n_leds; i++) {
        // extract red, green, blue from frame_buffer
        let r = frame_buffer[i][0];
        let g = frame_buffer[i][1];
        let b = frame_buffer[i][2];

        // adding master dimmer
        r *= dmx_dim_elem.value / 255; // mapping from 0-255 to 0-1
        g *= dmx_dim_elem.value / 255; // mapping from 0-255 to 0-1
        b *= dmx_dim_elem.value / 255; // mapping from 0-255 to 0-1

        // adding shutter
        if(shutter_closed) {
            r = 0;
            g = 0;
            b = 0;
        }

        // adding wave animation
        if(dmx_freq_elem.value > 0) {
            const scale = 0.5 * Math.sin(Math.PI * dmx_freq_elem.value / 255.0 * (i + wave_x_position)) + 0.5;
            r *= scale;
            g *= scale;
            b *= scale;
        }

        ctx.fillStyle = `rgb(${r}, ${g}, ${b})`;
        ctx.fillRect(i * 1800 / n_leds, 50, 1800 / n_leds, 1800 / n_leds);
    }
}

// HTML-Event: red, green, blue settings changed
let color_changed = () => {
    // update r, g, b to frame_buffer
    for(let i = 0; i < n_leds; i++) {
        frame_buffer[i] = [dmx_r_elem.value, dmx_g_elem.value, dmx_b_elem.value];
    }
    
    // display frame_buffer
    draw_leds();
}

// HTML-Event: master dimmer settings changed
let dimmer_changed = () => {
    draw_leds();
}

// HTML-Event: shutter settings changed
let shutter_changed = () => {
    if(dmx_shutter_elem.value == 0) { // disable shutter
        // stop shutter interval
        if(shutter_interval_handler != undefined) {
            clearInterval(shutter_interval_handler);
        }

        // open shutter
        shutter_closed = false;
        
        // display frame_buffer
        draw_leds();
    }
    else {
        // stop old interval
        if(shutter_interval_handler != undefined) {
            clearInterval(shutter_interval_handler);
        }

        // register new interval
        shutter_interval_handler = setInterval(() => {
            // toggle shutter
            shutter_closed = !shutter_closed;

            // display frame_buffer
            draw_leds();

        }, 1 / dmx_shutter_elem.value * 8000);
    }
}

// HTML-Event: wave animation settings changed
let wave_changed = () => {
    if(dmx_speed_elem.value == 0) { // disable wave animation
        // stop wave animation interval
        if(wave_interval_handler != undefined) {
            clearInterval(wave_interval_handler);
        }

        // reset wave position
        wave_x_position = 0;

        // display frame_buffer
        draw_leds();
    }
    else {
        // stop old interval
        if(wave_interval_handler != undefined) {
            clearInterval(wave_interval_handler);
        }

        // regitser new interval
        wave_interval_handler = setInterval(() => {
            // move wave animation
            wave_x_position -= 1;

            // display frame_buffer
            draw_leds();

        }, 1 / dmx_speed_elem.value * 8000);
    }
}