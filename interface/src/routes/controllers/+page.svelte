<script lang="ts">
	import ControllerRestComp from '$lib/components/ControllerRestComp.svelte';
	import ServoControllerRestComp from '$lib/components/ServoControllerRestComp.svelte';
import type { PageData } from '../$types';
	import Controllers from './Controllers.svelte';

	export let data: PageData;
</script>

<div
	class="mx-0 my-1 flex flex-col space-y-4
     sm:mx-8 sm:my-8"
>
{#if data.features.steppers}
	<ControllerRestComp
		label="Azimuth"
		restPath="/rest/heliostat/azimuth"
		>
	</ControllerRestComp>

	<ControllerRestComp 
		label="Elevation"
		restPath="/rest/heliostat/elevation"
		>
	</ControllerRestComp>
{/if}
{#if data.features.motors}
	<ServoControllerRestComp
		label="Azimuth Motor"
		restPath="/rest/heliostat/azimuth"
		>
	</ServoControllerRestComp>
	<ServoControllerRestComp
		label="Elevation Motor"
		restPath="/rest/heliostat/elevation"
		>
	</ServoControllerRestComp>
{/if}
</div>
