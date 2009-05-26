#
# Documentation build rules
#

report := manual.pdf
report_input := manual.tex applications.tex framework.tex conclusions.tex

all: $(report) slides.pdf

$(report): $(report_input)



.SUFFIXES:
.SUFFIXES: .pdf .tex
%.pdf: %.tex
	@set -e ; \
	echo '  PDFLaTeX  $<  [run 1]' ; \
	pdflatex -halt-on-error -file-line-error $< >/dev/null ; \
	echo '  PDFLaTeX  $<  [run 2]' ; \
	pdflatex -interaction nonstopmode $< >/dev/null ; \
	echo '  PDFLaTeX  $<  [run 3]' ; \
	pdflatex -interaction nonstopmode $< >/dev/null

.PHONY: clean
clean:
	@rm -fv *.aux *.out *.log *.lof *.lol *.toc *.bbl *.blg *.snm *.nav *.vrb $(informe)


