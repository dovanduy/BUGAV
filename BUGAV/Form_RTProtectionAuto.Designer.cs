﻿namespace BUGAV {
    partial class Form_RTProtectionAuto {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing) {
            if (disposing && (components != null)) {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent() {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form_RTProtectionAuto));
            this.RTAuto_NMF_Button = new DarkUI.Controls.DarkButton();
            this.RTAuto_notifyIcon = new System.Windows.Forms.NotifyIcon(this.components);
            this.ConsoleMonButton = new DarkUI.Controls.DarkButton();
            this.RTAutoConsole_notifyIcon = new System.Windows.Forms.NotifyIcon(this.components);
            this.WebApcMonButton = new DarkUI.Controls.DarkButton();
            this.ApiMonButton = new DarkUI.Controls.DarkButton();
            this.ApiMon_notifyIcon = new System.Windows.Forms.NotifyIcon(this.components);
            this.RTAuto_textBox = new System.Windows.Forms.TextBox();
            this.SuspendLayout();
            // 
            // RTAuto_NMF_Button
            // 
            this.RTAuto_NMF_Button.Location = new System.Drawing.Point(13, 13);
            this.RTAuto_NMF_Button.Name = "RTAuto_NMF_Button";
            this.RTAuto_NMF_Button.Padding = new System.Windows.Forms.Padding(5);
            this.RTAuto_NMF_Button.Size = new System.Drawing.Size(144, 23);
            this.RTAuto_NMF_Button.TabIndex = 0;
            this.RTAuto_NMF_Button.Text = "New Malware Files OFF";
            this.RTAuto_NMF_Button.Click += new System.EventHandler(this.darkButton1_Click);
            // 
            // RTAuto_notifyIcon
            // 
            this.RTAuto_notifyIcon.BalloonTipIcon = System.Windows.Forms.ToolTipIcon.Info;
            this.RTAuto_notifyIcon.BalloonTipText = "sdf";
            this.RTAuto_notifyIcon.BalloonTipTitle = "sdfsd";
            this.RTAuto_notifyIcon.Icon = ((System.Drawing.Icon)(resources.GetObject("RTAuto_notifyIcon.Icon")));
            this.RTAuto_notifyIcon.Text = "RTAuto_notifyIcon";
            // 
            // ConsoleMonButton
            // 
            this.ConsoleMonButton.Location = new System.Drawing.Point(13, 43);
            this.ConsoleMonButton.Name = "ConsoleMonButton";
            this.ConsoleMonButton.Padding = new System.Windows.Forms.Padding(5);
            this.ConsoleMonButton.Size = new System.Drawing.Size(144, 23);
            this.ConsoleMonButton.TabIndex = 1;
            this.ConsoleMonButton.Text = "ConsoleMonButton OFF";
            this.ConsoleMonButton.Click += new System.EventHandler(this.ConsoleMonButton_Click);
            // 
            // RTAutoConsole_notifyIcon
            // 
            this.RTAutoConsole_notifyIcon.BalloonTipIcon = System.Windows.Forms.ToolTipIcon.Warning;
            this.RTAutoConsole_notifyIcon.BalloonTipText = "asdf";
            this.RTAutoConsole_notifyIcon.BalloonTipTitle = "adsf";
            this.RTAutoConsole_notifyIcon.Icon = ((System.Drawing.Icon)(resources.GetObject("RTAutoConsole_notifyIcon.Icon")));
            this.RTAutoConsole_notifyIcon.Text = "RTAutoConsole_notifyIcon";
            // 
            // WebApcMonButton
            // 
            this.WebApcMonButton.Location = new System.Drawing.Point(13, 73);
            this.WebApcMonButton.Name = "WebApcMonButton";
            this.WebApcMonButton.Padding = new System.Windows.Forms.Padding(5);
            this.WebApcMonButton.Size = new System.Drawing.Size(144, 23);
            this.WebApcMonButton.TabIndex = 2;
            this.WebApcMonButton.Text = "WebApcMonButton OFF";
            this.WebApcMonButton.Click += new System.EventHandler(this.WebApcMonButton_Click);
            // 
            // ApiMonButton
            // 
            this.ApiMonButton.Location = new System.Drawing.Point(13, 103);
            this.ApiMonButton.Name = "ApiMonButton";
            this.ApiMonButton.Padding = new System.Windows.Forms.Padding(5);
            this.ApiMonButton.Size = new System.Drawing.Size(144, 23);
            this.ApiMonButton.TabIndex = 3;
            this.ApiMonButton.Text = "ApiMon OFF";
            this.ApiMonButton.Click += new System.EventHandler(this.ApiMonButton_Click);
            // 
            // ApiMon_notifyIcon
            // 
            this.ApiMon_notifyIcon.BalloonTipIcon = System.Windows.Forms.ToolTipIcon.Warning;
            this.ApiMon_notifyIcon.BalloonTipText = "asdfasdf";
            this.ApiMon_notifyIcon.BalloonTipTitle = "asdfa";
            this.ApiMon_notifyIcon.Icon = ((System.Drawing.Icon)(resources.GetObject("ApiMon_notifyIcon.Icon")));
            this.ApiMon_notifyIcon.Text = "ApiMon_notifyIcon";
            // 
            // RTAuto_textBox
            // 
            this.RTAuto_textBox.BackColor = System.Drawing.SystemColors.WindowFrame;
            this.RTAuto_textBox.ForeColor = System.Drawing.SystemColors.Info;
            this.RTAuto_textBox.Location = new System.Drawing.Point(164, 13);
            this.RTAuto_textBox.Multiline = true;
            this.RTAuto_textBox.Name = "RTAuto_textBox";
            this.RTAuto_textBox.Size = new System.Drawing.Size(624, 425);
            this.RTAuto_textBox.TabIndex = 4;
            // 
            // Form_RTProtectionAuto
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(800, 450);
            this.Controls.Add(this.RTAuto_textBox);
            this.Controls.Add(this.ApiMonButton);
            this.Controls.Add(this.WebApcMonButton);
            this.Controls.Add(this.ConsoleMonButton);
            this.Controls.Add(this.RTAuto_NMF_Button);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "Form_RTProtectionAuto";
            this.Text = "Form_RTProtectionAuto";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private DarkUI.Controls.DarkButton RTAuto_NMF_Button;
        public System.Windows.Forms.NotifyIcon RTAuto_notifyIcon;
        private DarkUI.Controls.DarkButton ConsoleMonButton;
        public System.Windows.Forms.NotifyIcon RTAutoConsole_notifyIcon;
        private DarkUI.Controls.DarkButton WebApcMonButton;
        private DarkUI.Controls.DarkButton ApiMonButton;
        public System.Windows.Forms.NotifyIcon ApiMon_notifyIcon;
        private System.Windows.Forms.TextBox RTAuto_textBox;
    }
}