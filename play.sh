ffplay -i output.yuv -pix_fmt yuv420p -s 1024x436
ffplay -f f32le -ac 2 -ar 44100 output.pcm