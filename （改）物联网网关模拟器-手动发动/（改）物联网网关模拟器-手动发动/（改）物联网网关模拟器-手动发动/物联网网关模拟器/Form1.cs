using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace 物联网网关模拟器
{
    public partial class Form1 : Form
    {
        private int[] br = new int[12] { 1200, 2400, 4800, 9600, 14400, 19200, 38400, 56000, 57600, 115200, 128000, 256000 };
        public Form1()
        {
            InitializeComponent();
        }
        private const int TOTAL_SENOSR_NUMBER = 9;
        private String[] titles = new String[4] {"传感器类型", "传感器数值" , "单位", "更新"};
        private String[] sensorTypes = new String[TOTAL_SENOSR_NUMBER] {"温度传感器", "湿度传感器", "光照传感器", "门", "灯", "空调", "窗帘" ,"扫地机器人","烟雾传感器"};
        private String[] units = new String[TOTAL_SENOSR_NUMBER] {"℃", "%", "lux", "", "", "", "%","" ,"mg/m^3"};
        private TaskTimer[] timers = new TaskTimer[10];
        private void Form1_Load(object sender, EventArgs e)
        {
            String[] ports = System.IO.Ports.SerialPort.GetPortNames();
            int i;
            comboBox1.Items.Clear();
            for (i = 0; i < ports.Length; i++)
            {
                comboBox1.Items.Add(ports[i]);
            }
            comboBox1.SelectedIndex = 0;

            comboBox2.Items.Clear();
            for (i = 0; i < br.Length; i++)
            {
                comboBox2.Items.Add(br[i]);
            }
            comboBox2.SelectedIndex = 9;

            for (i = 0; i < 4; i++)
            {
                Label title = new Label();
                title.Text = titles[i];
                title.AutoSize = true;
                title.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right | System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)));
                title.TextAlign = ContentAlignment.MiddleCenter;
                tableLayoutPanel1.Controls.Add(title, i, 0);
            }
            
            for (i = 1; i <= TOTAL_SENOSR_NUMBER; i++)
            {
                Label type = new Label();
                type.Text = sensorTypes[i-1];
                type.AutoSize = false;
                type.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right | System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)));
                type.TextAlign = ContentAlignment.MiddleCenter;
                tableLayoutPanel1.Controls.Add(type, 0, i);

                Label unit = new Label();
                unit.Text = units[i - 1];
                unit.AutoSize = false;
                unit.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right | System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)));
                unit.TextAlign = ContentAlignment.MiddleCenter;
                tableLayoutPanel1.Controls.Add(unit, 2, i);

                Button btn = new Button();
                btn.Text = "发送";
                btn.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.None)));
                btn.Click += new EventHandler(OnSendButtonClicked);
                tableLayoutPanel1.Controls.Add(btn, 3, i);
                switch(i)
                {
                    case 1:
                    case 2:
                    case 3:
                    case 7:
                    case 9:
                        TextBox value = new TextBox();
                        value.Width = 50;
                        value.Text = "0";
                        value.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.None)));
                        value.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
                        tableLayoutPanel1.Controls.Add(value, 1, i);
                        value.KeyPress += RestrictToDigit;
                        break;
                    case 4:
                    case 5:
                    case 6:
                    case 8:
                        CheckBox cb = new CheckBox();
                        cb.Checked = false;
                        cb.Text = "关";
                        cb.AutoSize = true;
                        cb.CheckedChanged += OnSensorStateChanged;
                        cb.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.None)));
                        tableLayoutPanel1.Controls.Add(cb, 1, i);
                        break;
                }
            }
        }
        private void RestrictToDigit(object sender, KeyPressEventArgs e)
        {
            TextBox t = (TextBox)sender;
            if (!Char.IsNumber(e.KeyChar) && !Char.IsPunctuation(e.KeyChar) && !Char.IsControl(e.KeyChar))
            {
                e.Handled = true;//消除不合适字符
            }
            else if (Char.IsPunctuation(e.KeyChar))
            {
                if (e.KeyChar != '.' || t.Text.Length == 0)//小数点
                {
                    e.Handled = true;
                }
                if (t.Text.LastIndexOf('.') != -1)
                {
                    e.Handled = true;
                }
            }
        }
        private void SendSensorData(int row)
        {
            byte[] tx = new byte[10];
            
            tx[0] = (byte)'X';
            tx[1] = (byte)'Y';
            tx[3] = (byte)(row - 1);
            switch(row)
            {
                case 1:
                case 2:
                case 3:
                case 7:
                case 9:
                    tx[2] = 5;
                    TextBox value = (TextBox)tableLayoutPanel1.GetControlFromPosition(1, row);
                    byte[] b = System.BitConverter.GetBytes(float.Parse(value.Text));
                    tx[4] = b[0];
                    tx[5] = b[1];
                    tx[6] = b[2];
                    tx[7] = b[3];
                    break;
                case 4:
                case 5:
                case 6:
                case 8:
                    CheckBox cb = (CheckBox)tableLayoutPanel1.GetControlFromPosition(1, row);
                    if (cb.Checked)
                        tx[4] = 1;
                    else
                        tx[4] = 0;
                    tx[2] = 2;
                    break;
                default:
                    textBox1.AppendText("错误：");
                    return;
            }
            tx[tx[2] + 3] = tx[2];
            for (int i = 3; i < tx[2] + 3; i++)
                tx[tx[2] + 3] += tx[i];
            if (serialPort1.IsOpen)
            {
                serialPort1.Write(tx, 0, tx[2] + 4);
                textBox1.AppendText(" 发送：");
                for (int i = 0; i < tx[2] + 4; i++)
                {
                    textBox1.AppendText(String.Format("{0:X2}" + " ", tx[i]));
                }
                textBox1.AppendText("\r\n");
               textBox1.ScrollToCaret();
            }
        }
        private void OnSendButtonClicked(object source, EventArgs e)
        {
            Button btn = (Button)source;
            int row = tableLayoutPanel1.GetRow((Control)source);
            SendSensorData(row);
        }
        private void OnSensorStateChanged(object sender, EventArgs e)
        {
            CheckBox cb = (CheckBox)sender;
            if (cb.Checked)
                cb.Text = "开";
            else
                cb.Text = "关";
        }
        private void comboBox1_Click(object sender, EventArgs e)
        {
            String[] ports = System.IO.Ports.SerialPort.GetPortNames();
            int i;
            comboBox1.Items.Clear();
            for (i = 0; i < ports.Length; i++)
            {
                comboBox1.Items.Add(ports[i]);
            }
            comboBox1.SelectedIndex = 0;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (button1.Text == "开始连接")
            {
                serialPort1.BaudRate = br[comboBox2.SelectedIndex];
                serialPort1.PortName = comboBox1.SelectedItem.ToString();
                if(!serialPort1.IsOpen)
                {
                    serialPort1.Open();
                    timer1.Enabled = true;
                }
                    
                button1.Text = "断开连接";
            } else
            {
                if (serialPort1.IsOpen)
                {
                    serialPort1.Close();
                    timer1.Enabled = false;
                }
                button1.Text = "开始连接";
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            for(int i = 1; i <= TOTAL_SENOSR_NUMBER; i++)
            {
                SendSensorData(i);
            }
        }

        private void button3_Click(object sender, EventArgs e)
        {
            textBox1.Text = "";
        }

        private void Form1_FormClosed(object sender, FormClosedEventArgs e)
        {
            if (serialPort1.IsOpen)
                serialPort1.Close();
        }
        private UInt16 recvCount = 0;
        private byte recvFlag = 0, checkSum = 0;
        private byte[] recvCmd = new byte[270];
        private void timer1_Tick(object sender, EventArgs e)
        {
            byte[] buf = new byte[65535];
            int i, cnt;
            if (serialPort1.IsOpen == true)
            {
                if (serialPort1.BytesToRead != 0)
                {
                    cnt = serialPort1.Read(buf, 0, serialPort1.BytesToRead);
                    
                    for(i = 0; i < cnt; i++)
                    {
                        recvCmd[recvCount++] = buf[i];
                        switch (recvFlag)
                        {
                            case 0:
                                if (buf[i] == 'X')
                                    recvFlag = 1;
                                else
                                    recvCount = 0;
                                break;
                            case 1:
                                if (buf[i] == 'Y')
                                {
                                    checkSum = 0;
                                    recvFlag = 2;
                                }
                                else
                                {
                                    recvFlag = 0;
                                    recvCount = 0;
                                }   
                                break;
                            case 2:
                                checkSum += buf[i];
                                if(recvCount >= recvCmd[2] + 3)
                                    recvFlag = 3;
                                break;
                            case 3:
                                if(checkSum == buf[i])
                                {
                                    textBox1.AppendText(" 接收：");
                                    for(int j = 0; j < recvCmd[2] + 4; j++)
                                        textBox1.AppendText(recvCmd[j].ToString("X2") + " ");
                                    textBox1.AppendText("\r\n");
                                    textBox1.ScrollToCaret();
                                    switch(recvCmd[3])
                                    {
                                        case 4:
                                        case 5:
                                        case 7:
                                            CheckBox cb = (CheckBox)tableLayoutPanel1.GetControlFromPosition(1, recvCmd[3] + 1);
                                            if (recvCmd[4] == 1)
                                                cb.Checked = true;
                                            else
                                                cb.Checked = false;
                                            break;
                                        case 6:
                                        case 8:
                                            TextBox value = (TextBox)tableLayoutPanel1.GetControlFromPosition(1, recvCmd[3] + 1);
                                            value.Text = System.BitConverter.ToSingle(recvCmd, 4).ToString(); 
                                            break;
                                    }
                                }
                                recvFlag = 0;
                                recvCount = 0;
                                break;
                        }
                    }
                }
            }
        }
    }
    public class TaskTimer : Timer
    {
        #region <变量>
        /// <summary>
        /// 定时器id
        /// </summary>
        private int id;
        /// <summary>
        /// 定时器参数
        /// </summary>
        private string param;
        public static int num;
        #endregion

        #region <属性>
        /// <summary>
        /// 定时器id属性
        /// </summary>
        public int ID
        {
            set { id = value; }
            get { return id; }
        }
        /// <summary>
        /// 定时器参数属性
        /// </summary>
        public string Param
        {
            set { param = value; }
            get { return param; }
        }

        #endregion
        ///<summary>
        /// 构造函数
        /// </summary>
        public TaskTimer() : base()
        {

        }
    }
}
