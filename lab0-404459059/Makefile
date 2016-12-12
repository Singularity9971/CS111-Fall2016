SHELL:=/bin/bash
default:
	@gcc lab0.c -o lab0
clean:
	@-rm lab0 *.tar.gz *.txt
dist:
	@tar -czf lab0-404459059.tar.gz lab0.c Makefile *.png README
check: default
	@touch tmp.txt
	@echo Testing >> tmp.txt
	@./lab0 --input tmp.txt --output tmp_out.txt || (echo 'Read/write test failed!')
	@cmp tmp.txt tmp_out.txt || (echo 'Read/write test failed, output and input are different!')
	@rm tmp.txt tmp_out.txt

	@./lab0 --catch --segfault 2>/dev/null || [ $$? -eq 3 ] || (echo 'Segfault test failed')
	@echo 'If no errors up till now, all tests passed!'

