#!/usr/bin/ruby

def clear
	clear_command = if Gem.win_platform?
						"cls"
					else
						"clear"
					end
	system clear_command
end

def input
	begin
		text = gets.chomp
	rescue Exception
		clear
		exit
	end
	text
end

begin 
	require "colorize"
rescue LoadError
	puts "Error: Colorize is not installed"
	puts "To install it, write this: gem install colorize"
	print "Do you want me to install it for you? (y/n): "
	choice = input
	if(choice == 'y')
		system "gem install colorize"
	end
	exit
end

folders = Array.new
Dir.foreach('.') do |folder|
	next if(folder == '.' or folder == '..' or File.file? folder)
	folders.push(folder)
end

while true do
	clear
	puts "Choose a " + "project".colorize(:magenta) + 
		 " to test. Write " + "'q'".colorize(:yellow) +
		 " to " + "quit".colorize(:red) + ": \n\n"
	i = 0
	folders.each do |folder|
		i += 1
		puts "(" + i.to_s.colorize(:green) + ") " + folder.colorize(:light_blue)
	end

	print "\n$> ".colorize(:yellow)
	choice = input
	if(Integer(choice) rescue nil)
		index = Integer(choice) - 1
		if(index >= 0 && index < folders.length)
			files = Dir.glob(folders[index] + "/test/test_*.rb")
			i = 0
			files.each do |file|
				i += 1

				clear
				line = "----------------------------------------------------"
					.colorize(:light_yellow)
				puts line
				label = "Testing " + folders[index].colorize(:light_blue)
				print label
				
				for _ in 0..(line.length - label.length - 
						files.length.to_s.length - i.to_s.length - 7) do
					print " "
				end
				puts "(" + i.to_s.colorize(:green) + " of " + 
					files.length.to_s.colorize(:green) + ")"
				puts line + "\n\n"

				system  "ruby ./" + file

				puts "\n\n" + line
				puts "Press " + "enter".colorize(:yellow) + " to " + 
					 "continue".colorize(:light_green) + 
					 ", " + "ctrl-c".colorize(:yellow) + " to " + "quit".colorize(:red)
				puts line
				input
			end
		end
	elsif(choice == 'q')
		break
	end
end

clear
