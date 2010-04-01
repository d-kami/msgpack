//
//MessagePack for Java
//
//Copyright (C) 2009 FURUHASHI Sadayuki
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

package jp.ac.tsukuba.cs.softlab.msgpack;

import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.BufferedReader;
import java.io.Writer;
import java.io.IOException;
import jp.ac.tsukuba.cs.softlab.msgpack.schema.SSchemaParser;
import jp.ac.tsukuba.cs.softlab.msgpack.schema.ClassGenerator;

public abstract class Schema {
	private String expression;
	private String name;

	public Schema(String name) {
		this.expression = expression;
		this.name = name;
	}

	public String getName() {
		return name;
	}

	public String getFullName() {
		return name;
	}

	public String getExpression() {
		return name;
	}

	public static Schema parse(InputStream input) throws IOException{
		BufferedReader reader = null;
		String text = null;
		
		try{
			String line;
			StringBuilder builder = new StringBuilder();
			
			reader = new BufferedReader(new InputStreamReader(input));
			
			while((line = reader.readLine()) != null)
				builder.append(line).append('\n');
			
			text = builder.toString();
		}catch(IOException e){
		    e.printStackTrace();
		}finally{
			try{
				if(reader != null)
					reader.close();

			}catch(IOException e){
				e.printStackTrace();
			}
		}
		
		if(text == null)
			return null;
		
		return Schema.parse(text);
	}
	
	public static Schema parse(String source) {
		return SSchemaParser.parse(source);
	}

	public static Schema load(String source) {
		return SSchemaParser.load(source);
	}

	public void write(Writer output) throws IOException {
		ClassGenerator.write(this, output);
	}

	public abstract void pack(Packer pk, Object obj) throws IOException;

	public abstract Object convert(Object obj) throws MessageTypeException;


	public Object createFromNil() {
		return null;
	}

	public Object createFromBoolean(boolean v) {
		throw new RuntimeException("type error");
	}

	public Object createFromByte(byte v) {
		throw new RuntimeException("type error");
	}

	public Object createFromShort(short v) {
		throw new RuntimeException("type error");
	}

	public Object createFromInt(int v) {
		throw new RuntimeException("type error");
	}

	public Object createFromLong(long v) {
		throw new RuntimeException("type error");
	}

	public Object createFromFloat(float v) {
		throw new RuntimeException("type error");
	}

	public Object createFromDouble(double v) {
		throw new RuntimeException("type error");
	}

	public Object createFromRaw(byte[] b, int offset, int length) {
		throw new RuntimeException("type error");
	}

	/* FIXME
	public Object createFromBoolean(boolean v) {
		throw MessageTypeException.schemaMismatch(this);
	}

	public Object createFromByte(byte v) {
		throw MessageTypeException.schemaMismatch(this);
	}

	public Object createFromShort(short v) {
		throw MessageTypeException.schemaMismatch(this);
	}

	public Object createFromInt(int v) {
		throw MessageTypeException.schemaMismatch(this);
	}

	public Object createFromLong(long v) {
		throw MessageTypeException.schemaMismatch(this);
	}

	public Object createFromFloat(float v) {
		throw MessageTypeException.schemaMismatch(this);
	}

	public Object createFromDouble(double v) {
		throw MessageTypeException.schemaMismatch(this);
	}

	public Object createFromRaw(byte[] b, int offset, int length) {
		throw MessageTypeException.schemaMismatch(this);
	}
	*/
}

