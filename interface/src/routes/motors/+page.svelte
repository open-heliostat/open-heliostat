<script lang="ts">
	import DcMotorControlForm from '$lib/components/DCMotorControlForm.svelte';
	import DcMotorRestComp from '$lib/components/DCMotorRestComp.svelte';
	import type { PageData } from '../$types';

	export let data: PageData;
</script>

<div
	class="mx-0 my-1 flex flex-col space-y-4
     sm:mx-8 sm:my-8"
>
	<DcMotorRestComp
		restPath={"/rest/heliostat/azimuth/motor"}
		name="Azimuth Motor"
	></DcMotorRestComp>
	<DcMotorRestComp
		restPath={"/rest/heliostat/elevation/motor"}
		name="Elevation Motor"
	></DcMotorRestComp>
</div>
