�ESRAM ���[�`���������݃n���h���ɐݒ�
�E���荞�݂��g�p�����V���A�����o��
�E������ӂ� FIX ������ FLASH �ɏĂ��āC�uSD �J�[�h��̃t�@�[���� SRAM �Ƀ��[�h�v���^�p�J�n
�E���荞��? DMA? �𗘗p���� SD �J�[�h R/W
�ESD �J�[�h�}�����o
�EUSB �ď�����
----����----
�EFLASH �̃��[�`���� SRAM �v���O��������R�[������d�g�݂��m��

----------------------------
�ԑ��E�^�R -> 5V-tolerant	4pin
�A�N�Z������R����		6pin
G �Z���T���s�v
lap shot -> open drain		��1.5 jack
wifi�����Ƃ���3.3V		4pin?

�E���̑� I/F
SD �J�[�h
���Z�b�g SW
input sw * 1
led output * 4

��
�ETSC��RTC 32bit
�Etacho/spd �� ���̓L���v�`���ōs����? TIM2,3
�E���C�Z���T�[ �� TSC+GPIO ������ or �^�C�}x2 ���̓L���v�`��
�EAD �� G �̓��C�����[�v�ŉ��Z����
�ESD ���� FW ���[�h (�ł��Ȃ�����)
  PIO �ł���������
�ESD �������� (�ł��Ȃ�����)
  �o�b�t�@�����܂����� (4KB ���炢?) DMA
�E1/16 �b���ƂɃV���A���o�� UART or SPI(WiFi)
----------------------------

����:
vsd2.eww �� Release �� Flash �p
            exec_sram �� SRAM ���s�p
vsd2_sram.eww �� Release �͓����Ȃ����ۂ�
                 exec_sram �� Flash �����N SRAM ���s�p

��sram �����N���̃V���{����d��`�G���[�̉��
\exec_sram\List\vsd2.map ������
bin/make_entry2.pl �����s����
�r���h����ƁC��d��`�V���{�������ׂă��X�g�A�b�v�����
������x bin/make_entry2.pl �����s����

��rom_entry.s ���� symbol �� weak �ɂ�����@���킩��Ȃ��D
(PUBWKEAK ���� EQU �œ{����)
