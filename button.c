/************************************************************
  * @brief   按键驱动
	* @param   RT_NULL
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  * @note    button.c
  ***********************************************************/
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>

#include "button.h"

/*******************************************************************
 *                          变量声明                               
 *******************************************************************/

static struct button* Head_Button = RT_NULL;


/*******************************************************************
 *                         函数声明     
 *******************************************************************/
static char *StrnCopy(char *dst, const char *src, rt_uint32_t n);
static void Print_Btn_Info(Button_t* btn);
static void Add_Button(Button_t* btn);


/************************************************************
  * @brief   按键创建
	* @param   name : 按键名称
	* @param   btn : 按键结构体
  * @param   read_btn_level : 按键电平读取函数，需要用户自己实现返回rt_uint8_t类型的电平
  * @param   btn_trigger_level : 按键触发电平
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  * @note    RT_NULL
  ***********************************************************/
void Button_Create(const char *name,
                  Button_t *btn, 
                  rt_uint8_t(*read_btn_level)(void),
                  rt_uint8_t btn_trigger_level)
{
  if( btn == RT_NULL)
  {
    RT_DEBUG_LOG(RT_DEBUG_THREAD,("struct button is RT_NULL!"));
    ASSERT(ASSERT_ERR);
  }
  
  memset(btn, 0, sizeof(struct button));  //清除结构体信息，建议用户在之前清除
 
  StrnCopy(btn->Name, name, BTN_NAME_MAX); /* 创建按键名称 */
  
  
  btn->Button_State = NONE_TRIGGER;           //按键状态
  btn->Button_Last_State = NONE_TRIGGER;      //按键上一次状态
  btn->Button_Trigger_Event = NONE_TRIGGER;   //按键触发事件
  btn->Read_Button_Level = read_btn_level;    //按键读电平函数
  btn->Button_Trigger_Level = btn_trigger_level;  //按键触发电平
  btn->Button_Last_Level = btn->Read_Button_Level(); //按键当前电平
  btn->Debounce_Time = 0;
  
  RT_DEBUG_LOG(RT_DEBUG_THREAD,("button create success!"));
  
  Add_Button(btn);          //创建的时候添加到单链表中
  
  Print_Btn_Info(btn);     //打印信息
 
}

/************************************************************
  * @brief   按键触发事件与回调函数映射链接起来
	* @param   btn : 按键结构体
	* @param   btn_event : 按键触发事件
  * @param   btn_callback : 按键触发之后的回调处理函数。需要用户实现
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  ***********************************************************/
void Button_Attach(Button_t *btn,Button_Event btn_event,Button_CallBack btn_callback)
{
  if( btn == RT_NULL)
  {
    RT_DEBUG_LOG(RT_DEBUG_THREAD,("struct button is RT_NULL!"));
//    ASSERT(ASSERT_ERR);       //断言
  }
  
  if(BUTTON_ALL_RIGGER == btn_event)
  {
    for(rt_uint8_t i = 0 ; i < number_of_event-1 ; i++)
      btn->CallBack_Function[i] = btn_callback; //按键事件触发的回调函数，用于处理按键事件
  }
  else
  {
    btn->CallBack_Function[btn_event] = btn_callback; //按键事件触发的回调函数，用于处理按键事件
  }
}

/************************************************************
  * @brief   删除一个已经创建的按键
	* @param   RT_NULL
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  * @note    RT_NULL
  ***********************************************************/
void Button_Delete(Button_t *btn)
{
  struct button** curr;
  for(curr = &Head_Button; *curr;) 
  {
    struct button* entry = *curr;
    if (entry == btn) 
    {
      *curr = entry->Next;
    } 
    else
    {
      curr = &entry->Next;
    }
  }
}

/************************************************************
  * @brief   获取按键触发的事件
	* @param   RT_NULL
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  ***********************************************************/
void Get_Button_EventInfo(Button_t *btn)
{
  //按键事件触发的回调函数，用于处理按键事件
  for(rt_uint8_t i = 0 ; i < number_of_event-1 ; i++)
  {
    if(btn->CallBack_Function[i] != 0)
    {
      RT_DEBUG_LOG(RT_DEBUG_THREAD,("Button_Event:%d",i));
    }      
  } 
}

rt_uint8_t Get_Button_Event(Button_t *btn)
{
  return (rt_uint8_t)(btn->Button_Trigger_Event);
}

/************************************************************
  * @brief   获取按键触发的事件
	* @param   RT_NULL
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  ***********************************************************/
rt_uint8_t Get_Button_State(Button_t *btn)
{
  return (rt_uint8_t)(btn->Button_State);
}

/************************************************************
  * @brief   按键周期处理函数
  * @param   btn:处理的按键
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  * @note    必须以一定周期调用此函数，建议周期为20~50ms
  ***********************************************************/
void Button_Cycle_Process(Button_t *btn)
{
  rt_uint8_t current_level = (rt_uint8_t)btn->Read_Button_Level();//获取当前按键电平
  
  if((current_level != btn->Button_Last_Level)&&(++(btn->Debounce_Time) >= BUTTON_DEBOUNCE_TIME)) //按键电平发生变化，消抖
  {
      btn->Button_Last_Level = current_level; //更新当前按键电平
      btn->Debounce_Time = 0;                 //确定了是按下
      
      //如果按键是没被按下的，改变按键状态为按下(首次按下/双击按下)
      if((btn->Button_State == NONE_TRIGGER)||(btn->Button_State == BUTTON_DOUBLE))
      {
        btn->Button_State = BUTTON_DOWM;
      }
      //释放按键
      else if(btn->Button_State == BUTTON_DOWM)
      {
        btn->Button_State = BUTTON_UP;
        RT_DEBUG_LOG(RT_DEBUG_THREAD,("释放了按键"));
      }
  }
  
  switch(btn->Button_State)
  {
    case BUTTON_DOWM :            // 按下状态
    {
      if(btn->Button_Last_Level == btn->Button_Trigger_Level) //按键按下
      {
        #if CONTINUOS_TRIGGER     //支持连续触发

        if(++(btn->Button_Cycle) >= BUTTON_CONTINUOS_CYCLE)
        {
          btn->Button_Cycle = 0;
          btn->Button_Trigger_Event = BUTTON_CONTINUOS; 
          TRIGGER_CB(BUTTON_CONTINUOS);    //连按
          RT_DEBUG_LOG(RT_DEBUG_THREAD,("连按"));
        }
        
        #else
        
        btn->Button_Trigger_Event = BUTTON_DOWM;
      
        if(++(btn->Long_Time) >= BUTTON_LONG_TIME)  //释放按键前更新触发事件为长按
        {
          #if LONG_FREE_TRIGGER
          
          btn->Button_Trigger_Event = BUTTON_LONG; 
          
          #else
          
          if(++(btn->Button_Cycle) >= BUTTON_LONG_CYCLE)    //连续触发长按的周期
          {
            btn->Button_Cycle = 0;
            btn->Button_Trigger_Event = BUTTON_LONG; 
            TRIGGER_CB(BUTTON_LONG);    //长按
          }
          #endif
          
          if(btn->Long_Time == 0xFF)  //更新时间溢出
          {
            btn->Long_Time = BUTTON_LONG_TIME;
          }
          RT_DEBUG_LOG(RT_DEBUG_THREAD,("长按"));
        }
          
        #endif
      }

      break;
    } 
    
    case BUTTON_UP :        // 弹起状态
    {
      if(btn->Button_Trigger_Event == BUTTON_DOWM)  //触发单击
      {
        if((btn->Timer_Count <= BUTTON_DOUBLE_TIME)&&(btn->Button_Last_State == BUTTON_DOUBLE)) // 双击
        {
          btn->Button_Trigger_Event = BUTTON_DOUBLE;
          TRIGGER_CB(BUTTON_DOUBLE);    
          RT_DEBUG_LOG(RT_DEBUG_THREAD,("双击"));
          btn->Button_State = NONE_TRIGGER;
          btn->Button_Last_State = NONE_TRIGGER;
        }
        else
        {
            btn->Timer_Count=0;
            btn->Long_Time = 0;   //检测长按失败，清0
          
          #if (SINGLE_AND_DOUBLE_TRIGGER == 0)
            TRIGGER_CB(BUTTON_DOWM);    //单击
          #endif
            btn->Button_State = BUTTON_DOUBLE;
            btn->Button_Last_State = BUTTON_DOUBLE;
          
        }
      }
      
      else if(btn->Button_Trigger_Event == BUTTON_LONG)
      {
        #if LONG_FREE_TRIGGER
          TRIGGER_CB(BUTTON_LONG);    //长按
        #else
          TRIGGER_CB(BUTTON_LONG_FREE);    //长按释放
        #endif
        btn->Long_Time = 0;
        btn->Button_State = NONE_TRIGGER;
        btn->Button_Last_State = BUTTON_LONG;
      } 
      
      #if CONTINUOS_TRIGGER
        else if(btn->Button_Trigger_Event == BUTTON_CONTINUOS)  //连按
        {
          btn->Long_Time = 0;
          TRIGGER_CB(BUTTON_CONTINUOS_FREE);    //连发释放
          btn->Button_State = NONE_TRIGGER;
          btn->Button_Last_State = BUTTON_CONTINUOS;
        } 
      #endif
      
      break;
    }
    
    case BUTTON_DOUBLE :
    {
      btn->Timer_Count++;     //时间记录 
      if(btn->Timer_Count>=BUTTON_DOUBLE_TIME)
      {
        btn->Button_State = NONE_TRIGGER;
        btn->Button_Last_State = NONE_TRIGGER;
      }
      #if SINGLE_AND_DOUBLE_TRIGGER
      
        if((btn->Timer_Count>=BUTTON_DOUBLE_TIME)&&(btn->Button_Last_State != BUTTON_DOWM))
        {
          btn->Timer_Count=0;
          TRIGGER_CB(BUTTON_DOWM);    //单击
          btn->Button_State = NONE_TRIGGER;
          btn->Button_Last_State = BUTTON_DOWM;
        }
        
      #endif

      break;
    }

    default :
      break;
  }
  
}

/************************************************************
  * @brief   遍历的方式扫描按键，不会丢失每个按键
	* @param   RT_NULL
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  * @note    此函数要周期调用，建议20-50ms调用一次
  ***********************************************************/
void Button_Process(void)
{
  struct button* pass_btn;
  for(pass_btn = Head_Button; pass_btn != RT_NULL; pass_btn = pass_btn->Next)
  {
      Button_Cycle_Process(pass_btn);
  }
}

/************************************************************
  * @brief   遍历按键
	* @param   RT_NULL
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  * @note    RT_NULL
  ***********************************************************/
void Search_Button(void)
{
  struct button* pass_btn;
  for(pass_btn = Head_Button; pass_btn != RT_NULL; pass_btn = pass_btn->Next)
  {
    RT_DEBUG_LOG(RT_DEBUG_THREAD,("button node have %s",pass_btn->Name));
  }
}

/************************************************************
  * @brief   处理所有按键回调函数
	* @param   RT_NULL
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  * @note    暂不实现
  ***********************************************************/
void Button_Process_CallBack(void *btn)
{
  rt_uint8_t btn_event = Get_Button_Event(btn);

  switch(btn_event)
  {
    case BUTTON_DOWM:
    {
      RT_DEBUG_LOG(RT_DEBUG_THREAD,("添加你的按下触发的处理逻辑"));
      break;
    }
    
    case BUTTON_UP:
    {
      RT_DEBUG_LOG(RT_DEBUG_THREAD,("添加你的释放触发的处理逻辑"));
      break;
    }
    
    case BUTTON_DOUBLE:
    {
      RT_DEBUG_LOG(RT_DEBUG_THREAD,("添加你的双击触发的处理逻辑"));
      break;
    }
    
    case BUTTON_LONG:
    {
      RT_DEBUG_LOG(RT_DEBUG_THREAD,("添加你的长按触发的处理逻辑"));
      break;
    }
    
    case BUTTON_LONG_FREE:
    {
      RT_DEBUG_LOG(RT_DEBUG_THREAD,("添加你的长按释放触发的处理逻辑"));
      break;
    }
    
    case BUTTON_CONTINUOS:
    {
      RT_DEBUG_LOG(RT_DEBUG_THREAD,("添加你的连续触发的处理逻辑"));
      break;
    }
    
    case BUTTON_CONTINUOS_FREE:
    {
      RT_DEBUG_LOG(RT_DEBUG_THREAD,("添加你的连续触发释放的处理逻辑"));
      break;
    }
      
  } 
}


/**************************** 以下是内部调用函数 ********************/

/************************************************************
  * @brief   拷贝指定长度字符串
	* @param   RT_NULL
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  * @note    RT_NULL
  ***********************************************************/
static char *StrnCopy(char *dst, const char *src, rt_uint32_t n)
{
  if (n != 0)
  {
    char *d = dst;
    const char *s = src;
    do
    {
        if ((*d++ = *s++) == 0)
        {
            while (--n != 0)
                *d++ = 0;
            break;
        }
    } while (--n != 0);
  }
  return (dst);
}

/************************************************************
  * @brief   打印按键相关信息
	* @param   RT_NULL
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  * @note    RT_NULL
  ***********************************************************/
static void Print_Btn_Info(Button_t* btn)
{
  
  RT_DEBUG_LOG(RT_DEBUG_THREAD,("button struct information:\n\
              btn->Name:%s \n\
              btn->Button_State:%d \n\
              btn->Button_Trigger_Event:%d \n\
              btn->Button_Trigger_Level:%d \n\
              btn->Button_Last_Level:%d \n\
              ",
              btn->Name,
              btn->Button_State,
              btn->Button_Trigger_Event,
              btn->Button_Trigger_Level,
              btn->Button_Last_Level));
  Search_Button();
}
/************************************************************
  * @brief   使用单链表将按键连接起来
	* @param   RT_NULL
  * @return  RT_NULL
  * @author  jiejie
  * @github  https://github.com/jiejieTop
  * @date    2018-xx-xx
  * @version v1.0
  * @note    RT_NULL
  ***********************************************************/
static void Add_Button(Button_t* btn)
{
  struct button *pass_btn = Head_Button;
  
  while(pass_btn)
  {
    pass_btn = pass_btn->Next;
  }
  
  btn->Next = Head_Button;
  Head_Button = btn;
}





