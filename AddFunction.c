/**
    @copyright (C) COPYRIGHT 2022 Fortiortech Shenzhen
    @file      AddFunction.c
    @author    Fortiortech  Appliction Team
    @since     Create:2022-07-13
    @date      Last modify:2022-07-14
    @note      Last modify author is Marcel He
    @brief     This file contains main function used for Motor Control.
*/
#include <MyProject.h>

/* Public variables --------------------------------------------------------- */
bool data isCtrlPowOn = false;              ///< 开关机控制
PWMINPUTCAL        xdata mcPwmInput;        ///< PWM捕获结构体变量
FOCCTRL            xdata mcFocCtrl;         ///< FOC电机控制相关结构体变量
MCRAMP             xdata mcRefRamp;         ///< 控制指令爬坡结构体相关变量
debugONOFFTypeDef  xdata debug_ONOFFTest;   ///< ONOFF启停测试小工具结构体变量
MC_CONTROL         xdata MCCtrl;

//uint8   NatureWinFlag = 0;

/**
    @brief        对变量取16位的绝对值
    @param[in]    value
    @return       绝对值
    @date         2022-07-13
*/
uint16 Abs_F16(int16 value)
{
    if (value < 0)
    {
        return (-value);
    }
    else
    {
        return (value);
    }
}

/**
    @brief        对变量取32位的绝对值
    @param[in]    value
    @return       绝对值
    @date         2022-07-13
*/
uint32 Abs_F32(int32 value)
{
    if (value < 0)
    {
        return (-value);
    }
    else
    {
        return (value);
    }
}

uint8 MOTOR_DIR           = Disable;
uint8 MOTOR_FR            = Disable;

//#ifdef ENABLE_IOKEY
unsigned char key_value = 0xff;

enum KEY_VALUE
{
  KEY1_VALUE,
  KEY2_VALUE,
  KEY3_VALUE,
  KEY4_VALUE,	
};

#define KEY_NUM 2
// 时间参数 (单位：ms)
#define DEBOUNCE_TIME 20      // 消抖时间
#define LONG_PRESS_TIME 1000  // 长按判定时间
#define RESPONSE_INTERVAL 10  // 响应间隔时间

// 按键状态机
typedef enum {
    KEY_STATE_IDLE,        // 空闲状态
    KEY_STATE_DEBOUNCE,    // 消抖检测
    KEY_STATE_PRESSED,     // 已按下
    KEY_STATE_LONG_PRESS   // 长按触发
} NKeyState;

// 按键数据结构
typedef struct {
    uint16 GPIO_Pin;
    NKeyState state;
    uint32 pressTime;
    uint8 isPressed;
    uint8 shortPressEvent;  // 短按事件标志
    uint8 longPressEvent;   // 长按事件标志
} Key_t;

//映射失败 20250722 FIX HERE
Key_t keys[KEY_NUM]; 

//无法遍历功能 20250722 FIX HERE
void Key_Init(void)
{
	   static uint8 i;
	   for( i=0; i<KEY_NUM; i++)
		 {
					keys[i].state 					= KEY_STATE_IDLE;
					keys[i].pressTime 			= 0;
					keys[i].isPressed 			= 0;
					keys[i].shortPressEvent = 0;
					keys[i].longPressEvent 	= 0;				 
		 }
		 	 
     keys[1].GPIO_Pin = SW_ONOFF;
     keys[2].GPIO_Pin = SW_LED;			 
}

uint8 Read_IO_VAL( uint8 key_num )
{
	   static uint8 val;
		
	   if( SW_ONOFF == 0 ||
         SW_LED   == 0 )
		 {
			    val = 1;  	 
		 }
		 else 
		 {
			    val = 0; 
          key_value = 0xff;			 
		 }		 
		 
		 return val;
}

void iokey_scan_per1ms(void)
{	
	   static uint8 i, currentState;
	
			    currentState = Read_IO_VAL( i );	

					if( !SW_ONOFF ) {key_value = KEY1_VALUE; i=0;}
					if( !SW_LED )   {key_value = KEY2_VALUE; i=1;}				 

          switch(keys[i].state) 
			    {
            case KEY_STATE_IDLE:
                if(currentState) 
								{  // 检测到按下信号
                    keys[i].state = KEY_STATE_DEBOUNCE;
                    keys[i].pressTime = 0;
                }
                break;
                
            case KEY_STATE_DEBOUNCE:
                if(currentState) 
								{
                    if(++keys[i].pressTime >= DEBOUNCE_TIME) 
										{
                        keys[i].state = KEY_STATE_PRESSED;
                        keys[i].isPressed = 1;
                        keys[i].pressTime = 0;
											
                    }
                } else 
								{
                    keys[i].state = KEY_STATE_IDLE;  // 抖动，返回空闲
                }
                break;
                
            case KEY_STATE_PRESSED:
                if(!currentState) 
								{  // 按键释放
                    keys[i].shortPressEvent = 1;     // 设置短按事件标志
                    keys[i].isPressed = 0;
                    keys[i].state = KEY_STATE_IDLE;
                } else 
							  {
                    if(++keys[i].pressTime >= LONG_PRESS_TIME) 
										{
                        keys[i].longPressEvent = 1;  // 设置长按事件标志
                        keys[i].state = KEY_STATE_LONG_PRESS;
                    }
                }
                break;
                
            case KEY_STATE_LONG_PRESS:
                if(!currentState) 
								{  
									  // 长按后释放
                    keys[i].isPressed = 0;
                    keys[i].state = KEY_STATE_IDLE;
                }
                break;			
		 }
}

void iokey_handler(void)
{
			// 检查KEY1事件
			if(keys[0].shortPressEvent) 
			{
					keys[0].shortPressEvent = 0;  // 清除标志
				
					// 执行KEY1短按操作
					FAN_PWM( );				
			}
    
			if(keys[0].longPressEvent) 
			{
					keys[0].longPressEvent = 0;  // 清除标志
				
					// 执行KEY1长按操作	
				  //20250723 先保存当前档位
				  DIR_FALG_CNT          = FAN_CNT;	
				  MOTOR_FR              = 1;
				
				  //停机超时 
				  if( FAN_CNT == 1 )
			             DIR_INTR = FAN_INTR_F;	
          else if( FAN_CNT == 2 )
			             DIR_INTR = FAN_INTR_S;		
					else if( FAN_CNT == 3 )
			             DIR_INTR = FAN_INTR_T;							
				
				  //关闭电机  
				  FAN_CNT               = 0;
				  IRControl.ONOFFStatus = 0;	
		}
    
    // 检查KEY2事件
    if(keys[1].shortPressEvent) 
			{
        keys[1].shortPressEvent = 0;  // 清除标志
			  LED_PWM(); //FIX HERE FOR PARAM2	
				
        // 执行KEY2短按操作
    }
    
    if(keys[1].longPressEvent) 
			{
        keys[1].longPressEvent = 0;  // 清除标志
				
        // 执行KEY2长按操作
		}
}

/* STOP MOTOR */
void Stop_MOTOR(void)
{
//	  #if ( CTRL_TYPE == KEY_MOTOR )
//			  Key_MOS_OFF;
//			
//				IRControl.ONOFFStatus = 0;
//				IRControl.FlagSpeed   = 0;
//				IRControl.ONOFFStatus = 0;
//				IRControl.TargetSpeed = 0;
//				MCCtrl.OldTargetValue = IRControl.TargetSpeed;			
//		#endif
	
		  //切换标志位
      IRControl.FlagFR      = Enable; 					 
      MOTOR_DIR 						= Enable;
			dir_count             = 0;
			MOTOR_FR              = 0;
					 
			//改变当前转向
			if (mcFRState.FR == CW)
						mcFRState.FR = CCW;
			else
			  		mcFRState.FR = CW;
	
}

/* TS01 LED PWM CTR */
/* param1:on/off  param2:led duty 50 */
//unsigned char LED_MODE = 0;
void LED_PWM( void )
{
	  LED_CNT++;

		if( LED_CNT > LED_MAX_GEAR )
		{
			  LED_CNT = 0;
		}
}

void FAN_PWM( void )
{
	   FAN_CNT++;

		 if( FAN_CNT > FAN_MAX_GEAR )
		{
					FAN_CNT 							= 0;
					IRControl.ONOFFStatus = 0;
		}
}

/**
    @brief        启停测试工具，用于测试启动可靠性
    @date         2022-07-14
*/
void ONOFF_Test(void)
{
    if (mcState == mcRun) // 开机状态
    {
        debug_ONOFFTest.TimeCnt++;
        
        if (debug_ONOFFTest.TimeCnt > ONOFFTEST_ON_TIME)
        {
        	  IRControl.FlagFR 						= 1;
            debug_ONOFFTest.Times++;                       // 启停次数+1
            debug_ONOFFTest.TimeCnt     = 0;
        }
    }
				
//		#if (FG_MODE == HARD_TIMFG_OUTPUT)
//    {
//				FGOutput();
//    }
//		#endif
    
		#if (FREnable ==1)
		{
				FRControl();
		}
		#endif
    
//		#if (BRAKE_CTRL == Enable)
//    {
//				Brake_Check();
//    }
//		#endif
		
}

/**
    @brief        调速信号处理包含：开关机控制、将调速信号处理成控制目标给定信号
    @date         2022-07-14
*/
void TargetRef_Process(void)
{
	    if( FAN_CNT  ) 
			{
				  IRControl.ONOFFStatus = 1;
				  MCCtrl.TargetValue    = IRControl.SpeedLevel[FAN_CNT]; 
			}	

			FRControl();		
}


/**
    @brief        外部闭环控制函数，示例代码提供 电流环，速度环，功率环，UQ控制示例代码,可根据需要自行修改
                 建议使用默认1ms周期运行
    @date         2022-07-14
*/
void Speed_response(void)
{
    static int8 OpenLoopIqIncCnt = 0;

    //遥控更新转速 20250508 IR 
//    if (IRControl.FlagSpeed)
//    {
//    	MCCtrl.TargetValue = IRControl.SpeedLevel[IRControl.TargetSpeed];//给定转速=速度转换【档位】
//    	IRControl.FlagSpeed = 0; //clear ir key respone flag 
//    }
    
    if ((mcState == mcRun) || (mcState == mcStop))
    {
        switch (mcFocCtrl.CtrlMode)
        {
            case 0:
            {
				#if (Open_Start_Mode == PLL_Start)
            		if ((mcFocCtrl.SpeedFlt > Motor_Loop_Speed) && (PLLfunction.PLLFunctionFlag > 1))
				#else
              	  if (mcFocCtrl.SpeedFlt > Motor_Loop_Speed) //>=MOTOR_LOOP_RPM
				#endif
                {
                    mcFocCtrl.Mode0HoldCnt++;
                    
                    if (mcFocCtrl.Mode0HoldCnt > 10)
                    {
                    	mcFocCtrl.Mode0HoldCnt = 0;
                        mcFocCtrl.CtrlMode  = 1;
                        FOC_QKP             = QKP;
                        FOC_QKI             = QKI;
                        FOC_DKP             = DKP;
                        FOC_DKI             = DKI;
                        
                        FOC_EKP = OBSW_KP_GAIN_RUN4;                                       // 估算器里的PI的KP
                        FOC_EKI = OBSW_KI_GAIN_RUN4;                                       // 估算器里的PI的KI
                        // 启动电流环与外环给定衔接
                        #if (Motor_Speed_Control_Mode == SPEED_LOOP_CONTROL)
                        {
                            if (mcFocCtrl.Start_Mode    == TAILWIND_START)
                            {
                                mcRefRamp.OutValue_float = mcBemf.BEMFSpeed;
                            }
                            else
                            {
                                mcRefRamp.OutValue_float = mcFocCtrl.SpeedFlt;
                            }
                        }
                        #elif (Motor_Speed_Control_Mode == POWER_LOOP_CONTROL)
                        {
                            mcRefRamp.OutValue_float = mcFocCtrl.PowerFlt;
                        }
                        #elif (Motor_Speed_Control_Mode == UQ_LOOP_CONTROL)
                        {
                            mcRefRamp.OutValue_float = mcFocCtrl.UqFlt;
                        }
                        #endif
                        PI2_Init();
                        mcFocCtrl.LoopTime          = LOOP_TIME;
                        mcRefRamp.IncValue          = RAMP_INC;
                        mcRefRamp.DecValue          = RAMP_DEC;
//                        mcFocCtrl.IqRef             = FOC_IQREF;
//                        mcFocCtrl.IqSpeedRef        = mcFocCtrl.IqRef;
                        FOC_IDREF                   = ID_RUN_CURRENT;     // D轴启动电流
                        PI1_UKH                     = mcFocCtrl.IqRef;
                        PI2_UKH                     = mcFocCtrl.IqRef;
                    }
                }
                else
                {
                	if (++OpenLoopIqIncCnt > 10)
                	{
                		OpenLoopIqIncCnt = 0;
                		if (mcFocCtrl.IqRef < (IQ_StartRamp_MAXCURRENT- IQ_StartRamp_ACCCURRENT))//不能正常切闭环时自加电流
                		{
                			mcFocCtrl.IqRef += IQ_StartRamp_ACCCURRENT;
                		}
                		else 
                		{
                			mcFocCtrl.IqRef = IQ_StartRamp_MAXCURRENT;
                		}
                		FOC_IQREF = mcFocCtrl.IqRef;
                	}
                    mcFocCtrl.Mode0HoldCnt = 0;
                }
            }
            break;
            
            case 1:
            {
                mcFocCtrl.LoopTime++;
                
                if (mcFocCtrl.LoopTime >= LOOP_TIME)
                {
                    mcFocCtrl.LoopTime = 0;
                    MCCtrl.ActualValue = Motor_Ramp(MCCtrl.TargetValue);             // 控制命令爬坡函数，用于实现调速信号之间平滑过渡
                    #if (Motor_Speed_Control_Mode == CURRENT_LOOP_CONTROL)
                    {
                        mcFocCtrl.IqRef = MCCtrl.ActualValue;
                        FOC_IQREF       = mcFocCtrl.IqRef;
                    }
                    #elif (Motor_Speed_Control_Mode == SPEED_LOOP_CONTROL)
                    {
  
												mcFocCtrl.IqRef = HW_One_PI(MCCtrl.ActualValue - mcFocCtrl.SpeedFlt);
												FOC_IQREF = mcFocCtrl.IqRef;                         
                    }
                    #elif (Motor_Speed_Control_Mode == POWER_LOOP_CONTROL)
                    {
                        mcFocCtrl.IqRef = HW_One_PI(MCCtrl.ActualValue - mcFocCtrl.PowerFlt);
                        FOC_IQREF       = mcFocCtrl.IqRef;
                    }
                    #elif (Motor_Speed_Control_Mode == UQ_LOOP_CONTROL)
                    {
                        mcFocCtrl.IqRef = HW_One_PI(MCCtrl.ActualValue - mcFocCtrl.UqFlt);
                        FOC_IQREF       = mcFocCtrl.IqRef;
                    }
                    #else
                    {
                        /* ------------------自定义闭环START--------------------- */
                        /* ------------------自定义闭环 END--------------------- */
                    }
                    #endif
                    
					#if (POWER_LIMIT_ENALBE)
                    {
                    	PI1_UKMAX = HW_One_PI2(MCCtrl.PowerLimitValue - mcFocCtrl.PowerFlt);   //Limiting power
                    }
					#endif
                    
					#if (SPEED_LIMIT_ENALBE)
                    {
                    	PI1_UKMAX = HW_One_PI2(MCCtrl.SpeedLimitValue - mcFocCtrl.SpeedFlt);   //Limiting power
                    }
					#endif
                }
            }
            break;
        }
    }
}

/**
    @brief        控制给定爬坡函数
                 以浮点进行计算，解决整数爬坡由于精度的影响，导致爬坡结果阶梯变化
                 函数控制周期默认为闭环控制周期，建议使用默认1ms周期运行
    @param[in]    ref 给定目标值
    @return       爬坡结果（int16）
    @date         2022-07-14
*/
int16 Motor_Ramp(int16 ref)
{
    mcRefRamp.RefValue = ref;             // 爬坡函数输入
    
	if ((mcRefRamp.OutValue_float + mcRefRamp.IncValue) < mcRefRamp.RefValue)
	{
		mcRefRamp.OutValue_float += mcRefRamp.IncValue;
	}
	else if (mcRefRamp.OutValue_float > (mcRefRamp.RefValue+ mcRefRamp.DecValue))
	{
		mcRefRamp.OutValue_float -= mcRefRamp.DecValue;
	}
	else
	{
		mcRefRamp.OutValue_float = mcRefRamp.RefValue;
	}
//    if (mcRefRamp.OutValue_float < mcRefRamp.RefValue)
//    {
//        if (mcRefRamp.OutValue_float + mcRefRamp.IncValue < mcRefRamp.RefValue)
//        {
//            mcRefRamp.OutValue_float += mcRefRamp.IncValue;
//        }
//        else
//        {
//            mcRefRamp.OutValue_float = mcRefRamp.RefValue;
//        }
//    }
//    else
//    {
//        if (mcRefRamp.OutValue_float - mcRefRamp.DecValue > mcRefRamp.RefValue)
//        {
//            mcRefRamp.OutValue_float -= mcRefRamp.DecValue;
//        }
//        else
//        {
//            mcRefRamp.OutValue_float = mcRefRamp.RefValue;
//        }
//    }
    
    return (int16)mcRefRamp.OutValue_float;   // 输出浮点数取整
}

/**
    @brief        启动ATO爬坡函数，用于静止启动时候对ATO进行爬坡，提高启动可靠性
    @date         2022-07-14
*/
void ATORamp(void)
{	
    if (mcState == mcRun)
    {
		#if(Open_Start_Mode == PLL_Start)
    	{
			if (PLLfunction.BEMFBase_Update_Count > 10)
			{
				if ((mcFocCtrl.IqRef < IQ_Start_CURRENT) && \
					  (PLLfunction.PLLFunctionFlag == 1))
				{
					mcFocCtrl.IqRef += 2;
					FOC_IQREF = mcFocCtrl.IqRef;
				}
	
//				if (FlashPara.FladUserCodeCheckBemf < 3)
//				{
					if (PLLfunction.BEMFBase < Forced_V_factor1_Max)
					{
						PLLfunction.BEMFBase = PLLfunction.BEMFBase + IncClimbingFactor;
					}
					else
					{
						PLLfunction.BEMFBase = Forced_V_factor1_Max;
					}
//				}
//				else
//				{
//					if (PLLfunction.BEMFBase < FlashPara.Factor1MaxTargetSave)
//					{
//						PLLfunction.BEMFBase = PLLfunction.BEMFBase + IncClimbingFactor;
//					}
//				}
	
				PLLfunction.BEMFBase_Update_Count = 0;
			}
	
			PLLfunction.BEMFBase_Update_Count++;
			
			FOC_EKP 								 = OBSW_KP_GAIN_RUN4;             // 估算器里的PI的KP
			FOC_EKI 								 = OBSW_KI_GAIN_RUN4;             // 估算器里的PI的KI
			mcFocCtrl.Flg_ATORampEnd = 1;            // ATO 爬坡结束
    	}
			#else
    	{
					if (mcFocCtrl.IqRef < IQ_Start_CURRENT) 
					{
							mcFocCtrl.IqRef += 2;
							FOC_IQREF 			=  mcFocCtrl.IqRef;
					}
    		
					if (mcFocCtrl.State_Count <= (ATO_RAMP_HOLDTIME - 3*ATO_RAMP_PERIOD) && \
						 (mcFocCtrl.Flg_ATORampEnd == 0))
					{
							FOC_EKP 								 = OBSW_KP_GAIN_RUN4;             // 估算器里的PI的KP
							FOC_EKI 								 = OBSW_KI_GAIN_RUN4;             // 估算器里的PI的KI
							mcFocCtrl.Flg_ATORampEnd = 1;            // ATO 爬坡结束
					}
					else if (mcFocCtrl.State_Count <= (ATO_RAMP_HOLDTIME - 2 *ATO_RAMP_PERIOD))
					{
							FOC_EKP = OBSW_KP_GAIN_RUN3;             // 估算器里的PI的KP
							FOC_EKI = OBSW_KI_GAIN_RUN3;             // 估算器里的PI的KI
					}
					else if (mcFocCtrl.State_Count <= (ATO_RAMP_HOLDTIME - 1 *ATO_RAMP_PERIOD))
					{
							FOC_EKP = OBSW_KP_GAIN_RUN2;             // 估算器里的PI的KP
							FOC_EKI = OBSW_KI_GAIN_RUN2;             // 估算器里的PI的KI
					}
					else if (mcFocCtrl.State_Count <= (ATO_RAMP_HOLDTIME - 100))
					{
							FOC_EKP = OBSW_KP_GAIN_RUN1;              // 估算器里的PI的KP
							FOC_EKI = OBSW_KI_GAIN_RUN1;              // 估算器里的PI的KI
					}
    	}
		  #endif
			
    }
}

/**
@brief        FR控制逻辑，反转存在时间控制，不会一直反转
@date         2022-07-14
*/
void FRControl(void)
{
	static bit FRlag = 0;

	//正反转切换，存在如下情况：
	//1、关机或错误状态下正反转切换，则直接改变当前旋转状态mcFRState.FR，下一次启动则生效
	//2、运行或启动状态，先关机并标记该状态，关机后进入Break状态直接改变旋转状态，
	//且标记时重新开机并恢复转速
	//3、正反转过程时关机，则保持关机状态，反转/正转切换后状态下一次开机后生效

	if (IRControl.FlagFR )
	{
			//切换方向先降速 20250723
			if (mcFocCtrl.SpeedFlt > (300))
			{
					mcFRState.OldFRTarSpeed = mcFRState.OldTargetSpeed;
			
					IRControl.TargetSpeed = IRControl.SpeedLevel[DIR_FALG_CNT];
			
					/*******************/
					FRlag = 1;
			}
			else  //降速后
			{
					//待机状态 先切换方向
					if ((mcState == mcReady) || (mcState == mcFault))
					{
							//若为运行时正反转切换则恢复到切换前的转速并开机
							if (mcFRState.FlagFR )
							{
									mcState = mcInit;
									IRControl.ONOFFStatus = 1;
									IRControl.TargetSpeed = mcFRState.OldTargetSpeed;
									IRControl.FlagSpeed = 1;
									mcFRState.FlagFR = 0;
									FRlag = 0;
							}

							IRControl.FlagFR = 0;
					}
					else if ((mcState == mcRun) || (mcState == mcStart)) //运行中 先关机
					{
							//控制关机
							IRControl.TargetSpeed = 0;
							IRControl.FlagSpeed   = 1;
							IRControl.ONOFFStatus = 0;
				
							//运行状态下正反转切换置位标记
							mcFRState.FlagFR = 1;
				 
					}
			}
		}
	else  //设定速度以下状态
	{
			if ((FRlag) || (IRControl.TargetSpeed == 3))
			{
					if (mcFRState.OldTargetSpeed == 3)
					{
							mcFRState.OldTargetSpeed = 2;
					}

					IRControl.TargetSpeed = mcFRState.OldTargetSpeed;
					IRControl.FlagSpeed = 1;
			
					FRlag = 0;
		  }
	}
}

/**
    @brief        默认1ms周期服务函数，运行信号采样，调速信号处理，闭环控制，故障检测,ATO爬坡函数
                 该函数运行于大循环中，由SYSTICK定时器间隔1ms触发运行。
    @date         2022-07-14
*/
void TickCycle_1ms(void)
{	
	static uint16 TailWindmcStateCnt =0;
    SetBit(ADC_CR, ADCBSY);           // 使能ADC的DCBUS采样
	
		if (mcState == mcTailWind) 
    {
				if(++TailWindmcStateCnt>=15000)
				{
						TailWindDetect.TailWindOverFlowStatus =1; //顺逆风计时溢出
						TailWindmcStateCnt =0;
				}
		}
		else
		{
				TailWindmcStateCnt =0;
		}
	   
    if ((mcState != mcInit) && \
			  (mcState != mcReady))
    {
        /* -----速度滤波，注意低通滤波器系数范围为0---127----- */
        mcFocCtrl.SpeedFlt  = LPFFunction2(FOC__EOME, mcFocCtrl.SpeedFlt, 50);/* -----估算器估算速度值滤波----- */ 
        mcFocCtrl.EMFsquare = Sqrt_alpbet(FOC__EALP, FOC__EBET); /* -----估算器估算反电动势值滤波----- */
        
        if ((mcState == mcRun) || \
  					(mcState == mcStart))
        {
            mcFocCtrl.Power    = FOC__POW <<1;    /* -----功率值滤波----- */
            mcFocCtrl.PowerFlt = LPFFunction2(mcFocCtrl.Power, mcFocCtrl.PowerFlt, 30);
        }
    }
    else
    {
        mcFocCtrl.SpeedFlt = 0;
        mcFocCtrl.PowerFlt = 0;
    }
    
    /* -----母线电流值采样----- */
    Power_Currt = (ADC7_DR);
    
    if (Power_Currt > mcCurOffset.Iw_busOffset)
    {
        Power_Currt   = Power_Currt - mcCurOffset.Iw_busOffset;
    }
    else
    {
        Power_Currt   = 0;
    }
    
    mcFocCtrl.mcADCCurrentbus  = LPFFunction2(Power_Currt, mcFocCtrl.mcADCCurrentbus, 32); /* -----母线电流值滤波----- */
    mcFocCtrl.mcDcbusFlt       = LPFFunction2(ADC2_DR, mcFocCtrl.mcDcbusFlt, 50); /* -----母线电压值滤波----- */
    mcFocCtrl.NTCTempFlt       = LPFFunction2(ADC3_DR, mcFocCtrl.mcDcbusFlt, 50); /* -----NTC电压值滤波----- */
    mcFocCtrl.UqFlt            = FOC__UQ;/* -----Q轴电压值滤波----- */
    mcFocCtrl.UdFlt            = FOC__UD;/* -----D轴电压值滤波----- */
  
    if (mcState == mcBemfCheck)
    {
    	mcFocCtrl.MotorPhaseADCValue = LPFFunction2(ADC10_DR, mcFocCtrl.MotorPhaseADCValue, 20);
        
        if ((mcFocCtrl.State_Count > BEMF_ESTI_TIME) && \
					  (mcFocCtrl.State_Count < (BEMF_CHECK_TIME - 50)))
        {
						if (mcFocCtrl.MotorPhaseADCValue > mcCurOffset.MotorPhaseADCValueOffset)
						{
								mcFocCtrl.MotorPhaseDetaADCValue = mcFocCtrl.MotorPhaseADCValue - mcCurOffset.MotorPhaseADCValueOffset;
						}
						else
						{
								mcFocCtrl.MotorPhaseDetaADCValue = 0;
						}

						if (mcFocCtrl.MotorPhaseDetaADCValueMin > mcFocCtrl.MotorPhaseDetaADCValue)
						{
								mcFocCtrl.MotorPhaseDetaADCValueMin = mcFocCtrl.MotorPhaseDetaADCValue;
						}

						if (mcFocCtrl.MotorPhaseDetaADCValueMax < mcFocCtrl.MotorPhaseDetaADCValue)
						{
								mcFocCtrl.MotorPhaseDetaADCValueMax = mcFocCtrl.MotorPhaseDetaADCValue;
						}

						if (mcFocCtrl.MotorPhaseDetaADCValueMax > mcFocCtrl.MotorPhaseDetaADCValueMin)
						{
								mcFocCtrl.MotorPhaseDetaADCValueMAxMinDet = mcFocCtrl.MotorPhaseDetaADCValueMax - mcFocCtrl.MotorPhaseDetaADCValueMin;
						}
						else
						{
								mcFocCtrl.MotorPhaseDetaADCValueMAxMinDet = 0;
						}
        }
        else if (mcFocCtrl.State_Count >= (BEMF_CHECK_TIME - 50))
        {
						mcFocCtrl.MotorPhaseDetaADCValueMax 		  = 0;
						mcFocCtrl.MotorPhaseDetaADCValueMAxMinDet = 0;
						mcFocCtrl.MotorPhaseDetaADCValueMin       = 300;
        }
    }

    /* 获取调速信号，不同调速模式(PWMMODE,NONEMODE,SREFMODE,KEYSCANMODE)的目标值修改 */
    TargetRef_Process();
		
    /* 启动ATO控制，环路响应，如速度环、转矩环、功率环等 */
    Speed_response();
		
    /* 故障保护函数功能，如过欠压保护、启动保护、缺相、堵转等 */
    Fault_Detection();
		
    /* 电机启动ATO爬坡函数处理  */
    ATORamp();		
    
    /* 电机状态机的时序处理 */
    if (mcFocCtrl.State_Count > 0)
    {
        mcFocCtrl.State_Count--;
    }
		
    if (mcPwmInput.MotorOnFilter > 0)
    {
        mcPwmInput.MotorOnFilter--;
    }
		
    if (mcPwmInput.MotorOffFilter > 0)
    {
        mcPwmInput.MotorOffFilter--;
    }
    
    /* -----电机RUN状态的时序处理----- */
    if (mcState == mcRun)
    {
				if (Time.mcRunStateCount < 61000)                                    //Run状态计时
				{
						Time.mcRunStateCount++;
				}
    }
    else
    {
    	Time.mcRunStateCount = 0;
    }
    
		#if (TAILWIND_MODE == FOCMethod)
    {
				FOCTailWindTimeLimit();
			
				CurrentZeroDetect();
    }
		#elif(TAILWIND_MODE == BEMFMethod)
    {
				if ((mcState == mcBemfCheck) || \
					  (mcState == mcTailWind))
				{
						if (mcFocCtrl.mcBemfTimerCnt < 16000) //卡16秒
						{
								mcFocCtrl.mcBemfTimerCnt++;
						}
						else
						{
								mcFocCtrl.mcBemfFlowFlag = 1; // 溢出标志
						}
				}
				else
				{
						mcFocCtrl.mcBemfFlowFlag = 0; // 溢出标志
						mcFocCtrl.mcBemfTimerCnt = 0;
				}
			}
			#endif
}

void PLLStateFunction(void)
{
		PLLfunction.test = PLLfunction.Theta_pre + 2;

		switch (PLLfunction.PLLFunctionFlag)
		{
				case 1:
				if (PLLfunction.count < 30000)
				{
						PLLfunction.count++;
				}

				PLLSoftwareFunction();

				break;

				case 2:

				PLLfunction.PLLFunctionFlag = 3;

				break;

				default:
				break;
	}
}

//ADC 电压检测 20250625
//void BAT_Voltage(void)
//{
//    if ( BAT.Voltage.DectDealyCnt < 100 )
//    {
//        BAT.Voltage.DectDealyCnt++;
//    }
//    else
//    {
//        if ( mcFaultSource == FaultNoSource )
//        {
////            /* 过充检测 */
////            if ( mcFocCtrl.mcDcbusFlt >   OVER_VOLTAGE_PROTECT )
//						if ( BAT_LVL1 > OVER_CHARG )	 //28V
//            {
//                BAT.Voltage.OverVoltDetecCnt += 1;
//                
//                if ( BAT.Voltage.OverVoltDetecCnt >= 300 )
//                {c
//                    BAT.Voltage.OverVoltDetecCnt  = 0;
//                    mcFaultSource                 = FaultOverVoltageDC;
//                }
//            }
//            else
//            {
//                if ( BAT.Voltage.OverVoltDetecCnt > 0 )
//                {
//                    BAT.Voltage.OverVoltDetecCnt--;
//                }
//            }
//				}		
//				
//		 }
//}		

//第二种ADC 采样读取 20250625
//    if (mcState == mcBemfCheck)
//    {
//    	mcFocCtrl.MotorPhaseADCValue = LPFFunction2(ADC10_DR, mcFocCtrl.MotorPhaseADCValue, 20);
//        
//        if ((mcFocCtrl.State_Count > BEMF_ESTI_TIME) && \
//					  (mcFocCtrl.State_Count < (BEMF_CHECK_TIME - 50)))
//        {
//						if (mcFocCtrl.MotorPhaseADCValue > mcCurOffset.MotorPhaseADCValueOffset)
//						{
//								mcFocCtrl.MotorPhaseDetaADCValue = mcFocCtrl.MotorPhaseADCValue - mcCurOffset.MotorPhaseADCValueOffset;
//						}
						

/*-------------------------------------------------------------------------------------------------
Function Name  : WatchDogConfig
Description    : 看门狗初始化配置函数
Date           : 2020-09-23
Parameter      : wdtArrValue: [输入] Value--定时时间，单位ms，最小定时时间8ms，最大定时1800ms
  ------------------------------------------------------------------------------------------------- */
void WatchDogConfig(uint16 wdtArrValue)
{
		SetBit(CCFG1, WDT_EN);                                                      //看门狗使能
		WDT_ARR = ((uint16)(65532 - (uint32)wdtArrValue * 32768 / 1000) >> 8);
		SetBit(WDT_CR, WDTRF);                                                      //看门狗初始化
}

/*-------------------------------------------------------------------------------------------------
Function Name  : WatchDogRefresh
Description    : 看门狗喂狗函数
Date           : 2020-09-23
Parameter      : None
    ------------------------------------------------------------------------------------------------- */
void WatchDogRefresh(void)
{
	SetBit(WDT_CR, WDTRF);                                                      //看门狗复位
}

