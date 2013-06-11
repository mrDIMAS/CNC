#pragma once


namespace soft {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

		using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::IO::Ports;

	/// <summary>
	/// Summary for Form1
	/// </summary>
	public ref class Form1 : public System::Windows::Forms::Form
	{
	public:
		Form1(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~Form1()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::Button^  button1;

	protected: 

	private: System::IO::Ports::SerialPort^  serialPort;
	private: System::Windows::Forms::TextBox^  outData;
	private: System::Windows::Forms::TextBox^  textBox1;
	private: System::Windows::Forms::Timer^  timer1;

	private: System::ComponentModel::IContainer^  components;

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>


#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->components = (gcnew System::ComponentModel::Container());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->serialPort = (gcnew System::IO::Ports::SerialPort(this->components));
			this->outData = (gcnew System::Windows::Forms::TextBox());
			this->textBox1 = (gcnew System::Windows::Forms::TextBox());
			this->timer1 = (gcnew System::Windows::Forms::Timer(this->components));
			this->SuspendLayout();
			// 
			// button1
			// 
			this->button1->Location = System::Drawing::Point(47, 212);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(195, 64);
			this->button1->TabIndex = 0;
			this->button1->Text = L"Send";
			this->button1->UseVisualStyleBackColor = true;
			this->button1->Click += gcnew System::EventHandler(this, &Form1::button1_Click);
			// 
			// serialPort
			// 
			this->serialPort->PortName = L"COM2";
			// 
			// outData
			// 
			this->outData->Location = System::Drawing::Point(51, 70);
			this->outData->Name = L"outData";
			this->outData->Size = System::Drawing::Size(190, 20);
			this->outData->TabIndex = 1;
			// 
			// textBox1
			// 
			this->textBox1->Location = System::Drawing::Point(56, 117);
			this->textBox1->Name = L"textBox1";
			this->textBox1->Size = System::Drawing::Size(185, 20);
			this->textBox1->TabIndex = 2;
			// 
			// timer1
			// 
			this->timer1->Enabled = true;
			this->timer1->Interval = 10;
			this->timer1->Tick += gcnew System::EventHandler(this, &Form1::timer1_Tick);
			// 
			// Form1
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(288, 307);
			this->Controls->Add(this->textBox1);
			this->Controls->Add(this->outData);
			this->Controls->Add(this->button1);
			this->Name = L"Form1";
			this->Text = L"Form1";
			this->Load += gcnew System::EventHandler(this, &Form1::Form1_Load);
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
		String ^received;
	private: System::Void Form1_Load(System::Object^  sender, System::EventArgs^  e) 
			 {
				 serialPort->ReadTimeout = 1;
				serialPort->Open();
				serialPort->DataReceived += gcnew System::IO::Ports::SerialDataReceivedEventHandler(this, &Form1::DataReceivedHandler);

			 }
	private: System::Void button1_Click(System::Object^  sender, System::EventArgs^  e) 
			 {
				 //String ^end = "\0";

				// outData->AppendText( end );

				 System::String ^cmd = (outData->Text);
				 
				 serialPort->WriteLine( cmd );
			 }
	private: System::Void button2_Click(System::Object^  sender, System::EventArgs^  e) {
				
			 }
private: System::Void DataReceivedHandler( Object^ sender, SerialDataReceivedEventArgs^ e)
		 {
			char byte = serialPort->ReadChar( );

			if( byte != 0 )
				received += byte;
		 }
private: System::Void timer1_Tick(System::Object^  sender, System::EventArgs^  e) {
			  textBox1->Text += received;
		 }
};
}

