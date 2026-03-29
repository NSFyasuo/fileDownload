#include "common.h" 
#include <math.h>

int rgb24to32(int color24)
{
    int r24,g24,b24,color32;
    r24=color24&0xff;
    g24=(color24>>8)&0xff;
    b24=(color24>>16)&0xff;
    color32=(r24<<16)|(g24<<8)|b24;
    return color32;
}

int *buf24tobuf16(unsigned char *buf24,short imgw,short imgh )
{
	int *buf32=malloc(imgw*imgh*sizeof(int));
	int i,j,color24;
	for(j=0;j<imgh;j++)
		for(i=0;i<imgw;i++)
			{
				color24=*(int *)buf24;
				buf32[j*imgw+i]=rgb24to32(color24);
				buf24+=3;
			}
	return buf32;
}

// 缩放函数（32位版本）
void scale32(int *buf32, short imgw, short imgh, int *scaled_buf32, int myw, int myh)
{
	int i,j,imgi,imgj;
	for(j=0; j<myh; j++)
	for(i=0; i<myw; i++)
	{
		imgi = i * imgw / myw;
		imgj = j * imgh / myh;
		scaled_buf32[j*myw+i] = buf32[imgj*imgw+imgi];
	}
}

void display(char *filename)
{
    int i,j;
    unsigned char *buf24;
    int color24;
    short imgw,imgh;
    //color16=(short *)image;
    buf24=decode_jpeg(filename, &imgw, &imgh);
    for(j=0;j<imgh;j++)
        for(i=0;i<imgw;i++)
            {
            //fb_point(i,j,rgb16to32(*color16));
            color24=*(int *)buf24;
            fb_point(i,j,rgb24to32(color24));  
            buf24+=3;
            }
        
}

void img_display_lr(char *filename, int myw, int myh)
{
	int i,j;
	unsigned char *buf24;
	short imgw,imgh;
	int *buf32;
    int *scaled_buf32;
    
	buf24=decode_jpeg(filename, &imgw, &imgh);
	buf32=buf24tobuf16(buf24,imgw,imgh);
    
    // 创建缩放后的缓冲区
    scaled_buf32 = malloc(myw * myh * sizeof(int));
    scale32(buf32, imgw, imgh, scaled_buf32, myw, myh);
    
	for(i=0; i<myw; i++)
	{
		for(j=0; j<myh; j++)
		{
			fb_point(i, j, scaled_buf32[j*myw+i]);
		}
		usleep(2000);
	}
    
    free(scaled_buf32);
    free(buf32);
}

void img_display_ud(char *filename, int myw, int myh)
{
	int i,j;
	unsigned char *buf24;
	short imgw,imgh;
	int *buf32;
    int *scaled_buf32;
    
	buf24=decode_jpeg(filename, &imgw, &imgh);
	buf32=buf24tobuf16(buf24,imgw,imgh);
    
    // 创建缩放后的缓冲区
    scaled_buf32 = malloc(myw * myh * sizeof(int));
    scale32(buf32, imgw, imgh, scaled_buf32, myw, myh);
    
	for(j=0; j<myh; j++)
	{
		for(i=0; i<myw; i++)
		{
			fb_point(i, j, scaled_buf32[j*myw+i]);
		}
		usleep(2000);
	}
    
    free(scaled_buf32);
    free(buf32);
}

void img_display_close(char *filename, int myw, int myh)
{
	int i,j,k;
	unsigned char *buf24;
	short imgw,imgh;
	int *buf32;
    int *scaled_buf32;
    
	buf24=decode_jpeg(filename, &imgw, &imgh);
	buf32=buf24tobuf16(buf24,imgw,imgh);
    
    // 创建缩放后的缓冲区
    scaled_buf32 = malloc(myw * myh * sizeof(int));
    scale32(buf32, imgw, imgh, scaled_buf32, myw, myh);
    
	for(i=0,k=myw-1; i<myw/2; i++,k--)
	{
		for(j=0; j<myh; j++)
		{
			fb_point(i, j, scaled_buf32[j*myw+i]);
			fb_point(k, j, scaled_buf32[j*myw+k]);
		}
		usleep(2000);
	}
    
    free(scaled_buf32);
    free(buf32);
}

void img_display_open(char *filename, int myw, int myh)
{
	int i,j,k;
	unsigned char *buf24;
	short imgw,imgh;
	int *buf32;
    int *scaled_buf32;
    
	buf24=decode_jpeg(filename, &imgw, &imgh);
	buf32=buf24tobuf16(buf24,imgw,imgh);
    
    // 创建缩放后的缓冲区
    scaled_buf32 = malloc(myw * myh * sizeof(int));
    scale32(buf32, imgw, imgh, scaled_buf32, myw, myh);
    
	for(i=k=myw/2; i>=0; i--,k++)
	{
		for(j=0; j<myh; j++)
		{
			fb_point(i, j, scaled_buf32[j*myw+i]);
			fb_point(k, j, scaled_buf32[j*myw+k]);
		}
		usleep(2000);
	}
    
    free(scaled_buf32);
    free(buf32);
}

//雪花溶解
void img_display_dissolve(char *filename, int myw, int myh)
{
    int i,j;
    unsigned char *buf24;
    short imgw,imgh;
    int *buf32;
    int *scaled_buf32;
    int total_pixels, displayed_pixels = 0;
    
    buf24=decode_jpeg(filename, &imgw, &imgh);
    buf32=buf24tobuf16(buf24,imgw,imgh);
    
    // 创建缩放后的缓冲区
    scaled_buf32 = malloc(myw * myh * sizeof(int));
    scale32(buf32, imgw, imgh, scaled_buf32, myw, myh);
    
    total_pixels = myw * myh;
    
    // 创建显示标记数组
    int *displayed = (int *)calloc(total_pixels, sizeof(int));
    
    // 随机显示像素
    srand(time(NULL));
    while(displayed_pixels < total_pixels)
    {
        int index = rand() % total_pixels;
        
        if(!displayed[index])
        {
            i = index % myw;
            j = index / myw;
            
            fb_point(i, j, scaled_buf32[index]);
            displayed[index] = 1;
            displayed_pixels++;
            
            if(displayed_pixels % 100 == 0) // 每显示100个像素稍作停顿
                usleep(500);
        }
    }
    
    free(displayed);
    free(scaled_buf32);
    free(buf32);
}


// 波浪扭曲特效（动态波浪后恢复正常）
void img_display_wave(char *filename, int myw, int myh)
{
    int i,j;
    unsigned char *buf24;
    short imgw,imgh;
    int *buf32;
    int *scaled_buf32;
    
    buf24=decode_jpeg(filename, &imgw, &imgh);
    buf32=buf24tobuf16(buf24,imgw,imgh);
    
    // 创建缩放后的缓冲区
    scaled_buf32 = malloc(myw * myh * sizeof(int));
    scale32(buf32, imgw, imgh, scaled_buf32, myw, myh);
    
    // 波浪动画：强波浪逐渐减弱
    for(int frame=0; frame<30; frame++)
    {
        float wave_strength = (30.0 - frame) / 30.0 * 15.0; // 从15逐渐减弱到0
        
        for(j=0; j<myh; j++)
        {
            for(i=0; i<myw; i++)
            {
                float wave_x = sin(j * 0.1 + frame * 0.3) * wave_strength;
                float wave_y = cos(i * 0.1 + frame * 0.2) * wave_strength * 0.5;
                
                int src_x = i + (int)wave_x;
                int src_y = j + (int)wave_y;
                
                if(src_x >= 0 && src_x < myw && src_y >= 0 && src_y < myh)
                {
                    fb_point(i, j, scaled_buf32[src_y*myw+src_x]);
                }
                else
                {
                    fb_point(i, j, 0);
                }
            }
        }
        usleep(50000);
    }
    
    // 最终显示完整正常图像
    for(j=0; j<myh; j++)
    {
        for(i=0; i<myw; i++)
        {
            fb_point(i, j, scaled_buf32[j*myw+i]);
        }
    }
    
    free(scaled_buf32);
    free(buf32);
}

// 径向渐变显示（从中心向外扩散显示完整图像）
void img_display_radial(char *filename, int myw, int myh)
{
    int i,j;
    unsigned char *buf24;
    short imgw,imgh;
    int *buf32;
    int *scaled_buf32;
    
    buf24=decode_jpeg(filename, &imgw, &imgh);
    buf32=buf24tobuf16(buf24,imgw,imgh);
    
    // 创建缩放后的缓冲区
    scaled_buf32 = malloc(myw * myh * sizeof(int));
    scale32(buf32, imgw, imgh, scaled_buf32, myw, myh);
    
    float center_x = myw / 2.0;
    float center_y = myh / 2.0;
    float max_distance = sqrt(center_x*center_x + center_y*center_y);
    
    // 从中心向外逐渐显示（动画效果）
    for(float radius=0; radius<=max_distance; radius+=max_distance/20.0)
    {
        for(j=0; j<myh; j++)
        {
            for(i=0; i<myw; i++)
            {
                float dx = i - center_x;
                float dy = j - center_y;
                float distance = sqrt(dx*dx + dy*dy);
                
                if(distance <= radius)
                {
                    fb_point(i, j, scaled_buf32[j*myw+i]);
                }
                else
                {
                    fb_point(i, j, 0); // 未显示区域为黑色
                }
            }
        }
        usleep(50000); // 控制显示速度
    }
    
    // 确保完全显示（最后一帧）
    for(j=0; j<myh; j++)
    {
        for(i=0; i<myw; i++)
        {
            fb_point(i, j, scaled_buf32[j*myw+i]);
        }
    }
    
    free(scaled_buf32);
    free(buf32);
}

// 漩涡特效（从漩涡扭曲逐渐恢复正常）
void img_display_swirl(char *filename, int myw, int myh)
{
    int i,j;
    unsigned char *buf24;
    short imgw,imgh;
    int *buf32;
    int *scaled_buf32;
    
    buf24=decode_jpeg(filename, &imgw, &imgh);
    buf32=buf24tobuf16(buf24,imgw,imgh);
    
    // 创建缩放后的缓冲区
    scaled_buf32 = malloc(myw * myh * sizeof(int));
    scale32(buf32, imgw, imgh, scaled_buf32, myw, myh);
    
    // 漩涡动画：从强漩涡逐渐减弱
    for(int frame=0; frame<=20; frame++)
    {
        float swirl_strength = (20.0 - frame) / 20.0 * 2.0; // 从2.0逐渐减弱到0
        
        float center_x = myw / 2.0;
        float center_y = myh / 2.0;
        float max_radius = sqrt(center_x*center_x + center_y*center_y);
        
        for(j=0; j<myh; j++)
        {
            for(i=0; i<myw; i++)
            {
                float dx = i - center_x;
                float dy = j - center_y;
                float distance = sqrt(dx*dx + dy*dy);
                
                if(distance < max_radius)
                {
                    float angle = (1.0 - distance/max_radius) * swirl_strength;
                    float cos_a = cos(angle);
                    float sin_a = sin(angle);
                    
                    int src_x = (int)(center_x + (dx * cos_a - dy * sin_a));
                    int src_y = (int)(center_y + (dx * sin_a + dy * cos_a));
                    
                    if(src_x >= 0 && src_x < myw && src_y >= 0 && src_y < myh)
                    {
                        fb_point(i, j, scaled_buf32[src_y*myw+src_x]);
                    }
                    else
                    {
                        fb_point(i, j, 0);
                    }
                }
                else
                {
                    fb_point(i, j, 0);
                }
            }
        }
        usleep(50000);
    }
    
    // 最终显示完整正常图像
    for(j=0; j<myh; j++)
    {
        for(i=0; i<myw; i++)
        {
            fb_point(i, j, scaled_buf32[j*myw+i]);
        }
    }
    
    free(scaled_buf32);
    free(buf32);
}

// 马赛克像素化特效（从马赛克逐渐变清晰）
void img_display_pixelate(char *filename, int myw, int myh)
{
    int i,j,block_i,block_j;
    unsigned char *buf24;
    short imgw,imgh;
    int *buf32;
    int *scaled_buf32;
    
    buf24=decode_jpeg(filename, &imgw, &imgh);
    buf32=buf24tobuf16(buf24,imgw,imgh);
    
    // 创建缩放后的缓冲区
    scaled_buf32 = malloc(myw * myh * sizeof(int));
    scale32(buf32, imgw, imgh, scaled_buf32, myw, myh);
    
    int max_block_size = 16; // 最大马赛克块大小
    int steps = 8; // 动画步骤
    
    // 从大马赛克逐渐变清晰
    for(int step=steps; step>=0; step--)
    {
        int current_block_size = (step * max_block_size) / steps;
        if(current_block_size < 1) current_block_size = 1;
        
        for(j=0; j<myh; j+=current_block_size)
        {
            for(i=0; i<myw; i+=current_block_size)
            {
                // 计算块的平均颜色
                int r_sum=0, g_sum=0, b_sum=0, count=0;
                
                for(block_j=j; block_j<j+current_block_size && block_j<myh; block_j++)
                {
                    for(block_i=i; block_i<i+current_block_size && block_i<myw; block_i++)
                    {
                        int color = scaled_buf32[block_j*myw+block_i];
                        r_sum += (color>>16) & 0xff;
                        g_sum += (color>>8) & 0xff;
                        b_sum += color & 0xff;
                        count++;
                    }
                }
                
                if(count > 0)
                {
                    int avg_color = ((r_sum/count)<<16) | ((g_sum/count)<<8) | (b_sum/count);
                    
                    // 填充整个块
                    for(block_j=j; block_j<j+current_block_size && block_j<myh; block_j++)
                    {
                        for(block_i=i; block_i<i+current_block_size && block_i<myw; block_i++)
                        {
                            fb_point(block_i, block_j, avg_color);
                        }
                    }
                }
            }
        }
        usleep(80000); // 控制动画速度
    }
    
    // 最终显示完整清晰图像
    for(j=0; j<myh; j++)
    {
        for(i=0; i<myw; i++)
        {
            fb_point(i, j, scaled_buf32[j*myw+i]);
        }
    }
    
    free(scaled_buf32);
    free(buf32);
}

// 碎片组合特效（图片以小碎片形式飞入组合）
void img_display_pieces(char *filename, int myw, int myh)
{
    int i,j;
    unsigned char *buf24;
    short imgw,imgh;
    int *buf32;
    int *scaled_buf32;
    
    buf24=decode_jpeg(filename, &imgw, &imgh);
    buf32=buf24tobuf16(buf24,imgw,imgh);
    
    // 创建缩放后的缓冲区
    scaled_buf32 = malloc(myw * myh * sizeof(int));
    scale32(buf32, imgw, imgh, scaled_buf32, myw, myh);
    
    int piece_size = 16; // 碎片大小
    int pieces_x = (myw + piece_size - 1) / piece_size;
    int pieces_y = (myh + piece_size - 1) / piece_size;
    int total_pieces = pieces_x * pieces_y;
    
    // 随机显示碎片
    int *piece_displayed = (int *)calloc(total_pieces, sizeof(int));
    srand(time(NULL));
    
    int displayed_count = 0;
    while(displayed_count < total_pieces)
    {
        int piece_idx = rand() % total_pieces;
        
        if(!piece_displayed[piece_idx])
        {
            int piece_x = (piece_idx % pieces_x) * piece_size;
            int piece_y = (piece_idx / pieces_x) * piece_size;
            
            // 显示这个碎片
            for(j=piece_y; j<piece_y+piece_size && j<myh; j++)
            {
                for(i=piece_x; i<piece_x+piece_size && i<myw; i++)
                {
                    fb_point(i, j, scaled_buf32[j*myw+i]);
                }
            }
            
            piece_displayed[piece_idx] = 1;
            displayed_count++;
            
            usleep(2500); // 每个碎片显示间隔
        }
    }
    
    free(piece_displayed);
    free(scaled_buf32);
    free(buf32);
}
